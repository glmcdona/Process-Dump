#include "test_support.h"
#include <map>
#include <sstream>
#include <thread>
#include <winternl.h>

namespace
{
	class owned_handle
	{
	public:
		HANDLE value;
		explicit owned_handle(HANDLE handle = NULL) : value(handle) {}
		~owned_handle() { if (value != NULL && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
		owned_handle(const owned_handle&) = delete;
		owned_handle& operator=(const owned_handle&) = delete;
	};

	void checked(bool success, const char* operation, DWORD error)
	{
		if (!success)
			throw std::runtime_error(std::string(operation) + ": Windows error " + std::to_string(error));
	}

	void checked(bool success, const char* operation)
	{
		checked(success, operation, GetLastError());
	}

	std::string quote(const std::string& text)
	{
		std::string result = "\"";
		size_t slashes = 0;
		for (char ch : text)
		{
			if (ch == '\\') { ++slashes; continue; }
			result.append(slashes * (ch == '"' ? 2 : 1), '\\');
			slashes = 0;
			if (ch == '"') result += '\\';
			result += ch;
		}
		result.append(slashes * 2, '\\');
		return result + '"';
	}

	std::string json_string(const std::string& text)
	{
		std::ostringstream output;
		output << '"';
		const char hex[] = "0123456789abcdef";
		for (unsigned char ch : text)
		{
			if (ch == '"' || ch == '\\') output << '\\' << ch;
			else if (ch < 32 || ch >= 127) output << "\\u00" << hex[ch >> 4] << hex[ch & 15];
			else output << ch;
		}
		output << '"';
		return output.str();
	}

	void write_report(const char* path, const std::string& text)
	{
		owned_handle file(CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
		checked(file.value != INVALID_HANDLE_VALUE, "Create report");
		DWORD written = 0;
		checked(WriteFile(file.value, text.data(), static_cast<DWORD>(text.size()), &written, NULL) &&
			written == text.size(), "Write report");
	}

	struct module
	{
		uintptr_t base;
		DWORD size;
		std::string name;
	};

	struct observation
	{
		bool loader = false, entry = false, idle = false, timeout = false;
		bool capture_attempted = false, captured = false;
		DWORD exit_code = STILL_ACTIVE, dump_exit = STILL_ACTIVE;
		uintptr_t image_base = 0;
		DWORD entry_rva = 0, dlls = 0, threads = 0, dropped_messages = 0;
		std::vector<std::string> markers, exceptions, loader_messages;

		std::string json() const
		{
			std::ostringstream out;
			out << "{\"loader_breakpoint\":" << loader << ",\"entrypoint\":" << entry
				<< ",\"input_idle\":" << idle << ",\"timed_out\":" << timeout
				<< ",\"capture_attempted\":" << capture_attempted << ",\"captured\":" << captured
				<< ",\"exit_code\":" << exit_code << ",\"dumper_exit_code\":" << dump_exit
				<< ",\"image_base\":" << image_base << ",\"entry_rva\":" << entry_rva
				<< ",\"loaded_dlls\":" << dlls << ",\"created_threads\":" << threads << ",\"markers\":[";
			for (size_t i = 0; i < markers.size(); ++i) out << (i ? "," : "") << json_string(markers[i]);
			out << "],\"exceptions\":[";
			for (size_t i = 0; i < exceptions.size(); ++i) out << (i ? "," : "") << exceptions[i];
			out << "],\"dropped_loader_messages\":" << dropped_messages << ",\"loader_messages\":[";
			for (size_t i = 0; i < loader_messages.size(); ++i) out << (i ? "," : "") << json_string(loader_messages[i]);
			out << "]}\n";
			return out.str();
		}
	};

	template <typename T> bool read_remote(HANDLE process, uintptr_t address, T& value)
	{
		SIZE_T read = 0;
		return ReadProcessMemory(process, reinterpret_cast<void*>(address), &value, sizeof(value), &read) &&
			read == sizeof(value);
	}

	bool image_header(HANDLE process, uintptr_t base, IMAGE_NT_HEADERS& header)
	{
		IMAGE_DOS_HEADER dos = {};
		return read_remote(process, base, dos) && dos.e_magic == IMAGE_DOS_SIGNATURE &&
			dos.e_lfanew > 0 && dos.e_lfanew < 1024 * 1024 &&
			read_remote(process, base + dos.e_lfanew, header) && header.Signature == IMAGE_NT_SIGNATURE &&
			header.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC;
	}

	void breakpoint_byte(HANDLE process, uintptr_t address, unsigned char byte)
	{
		DWORD protection = 0, ignored = 0;
		checked(VirtualProtectEx(process, reinterpret_cast<void*>(address), 1, PAGE_EXECUTE_READWRITE, &protection) != 0,
			"Protect owned child's entrypoint");
		SIZE_T written = 0;
		const bool success = WriteProcessMemory(process, reinterpret_cast<void*>(address), &byte, 1, &written) &&
			written == 1;
		const DWORD write_error = GetLastError();
		checked(VirtualProtectEx(process, reinterpret_cast<void*>(address), 1, protection, &ignored) != 0,
			"Restore owned child's entrypoint protection");
		checked(success, "Write owned child's entrypoint breakpoint", write_error);
		checked(FlushInstructionCache(process, reinterpret_cast<void*>(address), 1) != 0, "Flush entrypoint");
	}

	owned_handle* redirected_file(const std::string& path, STARTUPINFOA& startup, owned_handle& input)
	{
		SECURITY_ATTRIBUTES attributes = {sizeof(attributes), NULL, TRUE};
		std::unique_ptr<owned_handle> output(new owned_handle(CreateFileA(path.c_str(), GENERIC_WRITE,
			FILE_SHARE_READ, &attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)));
		checked(output->value != INVALID_HANDLE_VALUE, "Create child output");
		input.value = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&attributes, OPEN_EXISTING, 0, NULL);
		checked(input.value != INVALID_HANDLE_VALUE, "Open null input");
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_HIDE;
		startup.hStdInput = input.value;
		startup.hStdOutput = startup.hStdError = output->value;
		return output.release();
	}

	DWORD dump_child(HANDLE job, DWORD pid, uintptr_t base, const char* dumper, const char* folder,
		bool imports, const char* report)
	{
		std::ostringstream address;
		address << "0x" << std::hex << base;
		std::string command = quote(dumper) + " -pid " + std::to_string(pid) + " -a " + address.str() +
			" -o " + quote(folder) + " -db ignore -nep -nc -nt -nh -v" + (imports ? "" : " -ni");
		char repair[2] = {};
		if (GetEnvironmentVariableA("PD_REEXEC_PREPARE", repair, 2) == 1 && repair[0] == '1')
			command += " -reexec";
		STARTUPINFOA startup = {};
		owned_handle input;
		std::unique_ptr<owned_handle> output(redirected_file(std::string(report) + ".dump.log", startup, input));
		PROCESS_INFORMATION child = {};
		checked(CreateProcessA(dumper, &command[0], NULL, NULL, TRUE, CREATE_SUSPENDED | CREATE_NO_WINDOW,
			NULL, folder, &startup, &child) != 0, "Start dumper");
		owned_handle process(child.hProcess), thread(child.hThread);
		if (!AssignProcessToJobObject(job, process.value))
		{
			const DWORD error = GetLastError();
			checked(TerminateProcess(process.value, 1) != 0, "Stop unassigned dumper");
			checked(false, "Assign dumper job", error);
		}
		checked(ResumeThread(thread.value) != MAXDWORD, "Resume dumper");
		const DWORD wait = WaitForSingleObject(process.value, 30000);
		if (wait == WAIT_TIMEOUT)
		{
			checked(TerminateProcess(process.value, WAIT_TIMEOUT) != 0, "Stop timed-out dumper");
			checked(WaitForSingleObject(process.value, 5000) == WAIT_OBJECT_0, "Wait for stopped dumper");
			return WAIT_TIMEOUT;
		}
		checked(wait == WAIT_OBJECT_0, "Wait for dumper");
		DWORD exit = 0;
		checked(GetExitCodeProcess(process.value, &exit) != 0, "Read dumper status");
		return exit;
	}

	observation observe(int argc, char** argv)
	{
		// exe, report, duration-ms, capture-phase, dumper, dump-folder, imports, target arguments...
		const char *exe = argv[0], *report = argv[1], *phase = argv[3];
		char snaps_value[2] = {};
		const bool loader_snaps = GetEnvironmentVariableA("PD_REEXEC_LOADER_SNAPS", snaps_value, 2) == 1 &&
			snaps_value[0] == '1';
		char* end = NULL;
		const unsigned long duration = strtoul(argv[2], &end, 10);
		require(end != argv[2] && *end == 0 && duration >= 100 && duration <= 30000, "duration must be 100..30000 ms");
		require(strcmp(phase, "none") == 0 || strcmp(phase, "entry") == 0 ||
			strcmp(phase, "ready") == 0 || strcmp(phase, "idle") == 0, "unsupported capture phase");
		require(strcmp(argv[6], "0") == 0 || strcmp(argv[6], "1") == 0, "imports must be 0 or 1");
		DWORD binary_type = 0;
		checked(GetBinaryTypeA(exe, &binary_type) != 0, "Read executable type");
		require(binary_type == (sizeof(void*) == 8 ? SCS_64BIT_BINARY : SCS_32BIT_BINARY),
			"use the probe matching the executable architecture");
		std::string command = quote(exe);
		for (int i = 7; i < argc; ++i) command += " " + quote(argv[i]);
		STARTUPINFOA startup = {};
		owned_handle input;
		std::unique_ptr<owned_handle> output(redirected_file(std::string(report) + ".stdout", startup, input));
		owned_handle job(CreateJobObjectA(NULL, NULL));
		checked(job.value != NULL, "Create owned-child job");
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
		limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		checked(SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) != 0,
			"Set owned-child job cleanup");
		PROCESS_INFORMATION child = {};
		checked(CreateProcessA(exe, &command[0], NULL, NULL, TRUE,
			DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &startup, &child) != 0,
			"Start owned debug child");
		owned_handle process(child.hProcess), thread(child.hThread);
		if (!AssignProcessToJobObject(job.value, process.value))
		{
			const DWORD error = GetLastError();
			checked(TerminateProcess(process.value, 1) != 0, "Stop unassigned debug child");
			checked(false, "Assign owned-child job", error);
		}
		checked(ResumeThread(thread.value) != MAXDWORD, "Resume owned debug child");
		observation result;
		std::map<uintptr_t, module> modules;
		uintptr_t entry = 0;
		unsigned char original_byte = 0;
		bool armed = false, stopped = false, exited = false;
		const ULONGLONG started = GetTickCount64();
		ULONGLONG paused = 0, stopping = 0;
		const auto capture = [&]() {
			result.capture_attempted = true;
			const ULONGLONG before = GetTickCount64();
			result.dump_exit = dump_child(job.value, child.dwProcessId, result.image_base, argv[4], argv[5],
				strcmp(argv[6], "1") == 0, report);
			paused += GetTickCount64() - before;
			result.captured = result.dump_exit == 0;
		};
		while (!exited)
		{
			if (!stopped && GetTickCount64() - started - paused >= duration)
			{
				result.timeout = true;
				checked(TerminateJobObject(job.value, WAIT_TIMEOUT) != 0, "Stop owned timed-out children");
				stopped = true;
				stopping = GetTickCount64();
			}
			require(!stopped || GetTickCount64() - stopping < 10000, "debug child cleanup exceeded deadline");
			DEBUG_EVENT event = {};
			if (!WaitForDebugEvent(&event, 50))
			{
				checked(GetLastError() == ERROR_SEM_TIMEOUT, "Wait for owned child debug event");
				if (!stopped && result.entry && WaitForInputIdle(process.value, 0) == 0)
				{
					result.idle = true;
					if (!result.capture_attempted && strcmp(phase, "idle") == 0) capture();
				}
				continue;
			}
			DWORD disposition = DBG_CONTINUE;
			if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT || event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
			{
				const bool main = event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT;
				owned_handle file(main ? event.u.CreateProcessInfo.hFile : event.u.LoadDll.hFile);
				const uintptr_t base = reinterpret_cast<uintptr_t>(main ?
					event.u.CreateProcessInfo.lpBaseOfImage : event.u.LoadDll.lpBaseOfDll);
				IMAGE_NT_HEADERS header = {};
				const bool valid = image_header(process.value, base, header);
				char path[MAX_PATH] = {};
				const DWORD length = GetFinalPathNameByHandleA(file.value, path, MAX_PATH, 0);
				std::string name = length > 0 && length < MAX_PATH ? path : "unknown";
				const size_t slash = name.find_last_of("\\/");
				if (slash != std::string::npos) name.erase(0, slash + 1);
				modules[base] = {base, valid ? header.OptionalHeader.SizeOfImage : 0, name};
				if (main)
				{
					if (loader_snaps)
					{
						PROCESS_BASIC_INFORMATION basic = {};
						require(NtQueryInformationProcess(process.value, ProcessBasicInformation, &basic,
							sizeof(basic), NULL) >= 0, "query owned-child PEB failed");
						// Diagnostic-only Windows x86/x64 PEB layout; never writes a system-wide setting.
						const uintptr_t flags_address = reinterpret_cast<uintptr_t>(basic.PebBaseAddress) +
							(sizeof(void*) == 8 ? 0xbc : 0x68);
						DWORD flags = 0;
						checked(read_remote(process.value, flags_address, flags), "Read owned-child loader flags");
						flags |= 2; // FLG_SHOW_LDR_SNAPS
						SIZE_T written = 0;
						checked(WriteProcessMemory(process.value, reinterpret_cast<void*>(flags_address), &flags,
							sizeof(flags), &written) && written == sizeof(flags), "Set owned-child loader diagnostics");
					}
					require(valid && header.OptionalHeader.AddressOfEntryPoint != 0 &&
						header.OptionalHeader.AddressOfEntryPoint < header.OptionalHeader.SizeOfImage, "invalid native entrypoint");
					result.image_base = base;
					result.entry_rva = header.OptionalHeader.AddressOfEntryPoint;
					entry = base + result.entry_rva;
					checked(read_remote(process.value, entry, original_byte), "Read child entrypoint byte");
					require(original_byte != 0xcc, "entrypoint already contains a breakpoint");
					breakpoint_byte(process.value, entry, 0xcc);
					armed = true;
				}
				else ++result.dlls;
			}
			else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) ++result.threads;
			else if (event.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT)
				modules.erase(reinterpret_cast<uintptr_t>(event.u.UnloadDll.lpBaseOfDll));
			else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
			{
				const auto& exception = event.u.Exception.ExceptionRecord;
				const uintptr_t address = reinterpret_cast<uintptr_t>(exception.ExceptionAddress);
				std::string name = "unknown";
				uintptr_t rva = address;
				auto found = modules.upper_bound(address);
				if (found != modules.begin())
				{
					--found;
					if (address - found->first < found->second.size)
					{
						name = found->second.name;
						rva = address - found->first;
					}
				}
				if (exception.ExceptionCode == EXCEPTION_BREAKPOINT && armed && address == entry)
				{
					checked(event.dwThreadId == child.dwThreadId, "Unexpected entrypoint thread");
					breakpoint_byte(process.value, entry, original_byte);
					CONTEXT context = {};
					context.ContextFlags = CONTEXT_CONTROL;
					checked(GetThreadContext(thread.value, &context) != 0, "Read entrypoint context");
#ifdef _WIN64
					context.Rip = entry;
#else
					context.Eip = static_cast<DWORD>(entry);
#endif
					checked(SetThreadContext(thread.value, &context) != 0, "Restore entrypoint instruction pointer");
					armed = false;
					result.entry = true;
					if (strcmp(phase, "entry") == 0) capture();
				}
				else if (exception.ExceptionCode == EXCEPTION_BREAKPOINT && event.u.Exception.dwFirstChance &&
					!result.loader && !result.entry && _stricmp(name.c_str(), "ntdll.dll") == 0)
					result.loader = true;
				else
				{
					disposition = DBG_EXCEPTION_NOT_HANDLED;
					if (result.exceptions.size() < 32)
					{
						std::ostringstream item;
						item << "{\"code\":" << exception.ExceptionCode << ",\"first_chance\":"
							<< event.u.Exception.dwFirstChance << ",\"module\":" << json_string(name)
							<< ",\"rva\":" << rva << ",\"parameters\":[";
						for (DWORD i = 0; i < exception.NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i)
							item << (i ? "," : "") << exception.ExceptionInformation[i];
						item << "]}";
						result.exceptions.push_back(item.str());
					}
					if (!event.u.Exception.dwFirstChance && !stopped)
					{
						checked(TerminateJobObject(job.value, exception.ExceptionCode) != 0, "Stop failed owned child");
						stopped = true;
						stopping = GetTickCount64();
					}
				}
			}
			else if (event.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
			{
				const auto& info = event.u.DebugString;
				if (!info.fUnicode && info.nDebugStringLength > 0 && info.nDebugStringLength <= 4096)
				{
					char text[4097] = {};
					SIZE_T read = 0;
					if (ReadProcessMemory(process.value, info.lpDebugStringData, text, info.nDebugStringLength, &read) &&
						read == info.nDebugStringLength)
					{
						if (loader_snaps)
						{
							if (result.loader_messages.size() == 256)
							{
								result.loader_messages.erase(result.loader_messages.begin() + 32);
								++result.dropped_messages;
							}
							result.loader_messages.push_back(text);
						}
						if (strncmp(text, "PD_REEXEC_", 10) == 0)
						{
							if (result.markers.size() < 32) result.markers.push_back(text);
							if (!result.capture_attempted && strcmp(phase, "ready") == 0 && strcmp(text, "PD_REEXEC_READY") == 0)
								capture();
						}
					}
				}
			}
			else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
			{
				result.exit_code = event.u.ExitProcess.dwExitCode;
				exited = true;
			}
			checked(ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition) != 0, "Continue owned debug child");
		}
		return result;
	}
}

int run_reexecution_probe(int argc, char** argv)
{
	try
	{
		require(argc >= 7, "usage: --reexecute exe report duration-ms none|entry|ready|idle dumper dump-folder 0|1 [args]");
		const observation result = observe(argc, argv);
		write_report(argv[1], result.json());
		return 0;
	}
	catch (const std::exception& error)
	{
		fprintf(stderr, "Reexecution probe failed: %s\n", error.what());
		return 1;
	}
}

int run_reexecution_fixture()
{
	OutputDebugStringA("PD_REEXEC_READY");
	std::thread worker([] { OutputDebugStringA("PD_REEXEC_THREAD"); });
	worker.join();
	puts("PD_REEXEC_OK");
	OutputDebugStringA("PD_REEXEC_DONE");
	return 0;
}
