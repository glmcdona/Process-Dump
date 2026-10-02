#include "test_support.h"
#include "dump_path.h"
#include "dump_process.h"
#include "module_list.h"

namespace
{
	struct export_fixture
	{
		std::vector<unsigned char> bytes;
		static const SIZE_T directory_offset = 16;
		static const unsigned __int64 image_base = 0x400000;

		export_fixture() : bytes(256, 0)
		{
			IMAGE_EXPORT_DIRECTORY* entry = directory();
			entry->Name = 96;
			entry->Base = 1;
			entry->NumberOfFunctions = 1;
			entry->NumberOfNames = 1;
			entry->AddressOfFunctions = 80;
			entry->AddressOfNames = 84;
			entry->AddressOfNameOrdinals = 88;
			const DWORD rva = 0x180, name_offset = 248;
			const WORD ordinal = 0;
			memcpy(bytes.data() + 80, &rva, sizeof(rva));
			memcpy(bytes.data() + 84, &name_offset, sizeof(name_offset));
			memcpy(bytes.data() + 88, &ordinal, sizeof(ordinal));
			memcpy(bytes.data() + 96, "ordinary.dll", sizeof("ordinary.dll"));
			memcpy(bytes.data() + 248, "Example", sizeof("Example"));
		}

		IMAGE_EXPORT_DIRECTORY* directory()
		{
			return reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(bytes.data() + directory_offset);
		}

		bool parse(export_list& exports, SIZE_T length)
		{
			return exports.add_exports(bytes.data(), length, image_base, directory(), false);
		}
	};

	void export_bounded_strings()
	{
		export_fixture fixture;
		export_list exports;
		require(fixture.parse(exports, fixture.bytes.size()), "terminated export at image boundary rejected");
		char library[] = "ordinary.dll", name[] = "Example";
		require(exports.find_export(library, name, false) == export_fixture::image_base + 0x180,
			"bounded export lookup changed");
		export_entry entry = exports.find(export_fixture::image_base + 0x180);
		require(strcmp(entry.library_name, library) == 0 && strcmp(entry.name, name) == 0,
			"export strings were not copied exactly");

		export_list missing_name_terminator;
		require(!fixture.parse(missing_name_terminator, fixture.bytes.size() - 1),
			"export name without an in-range terminator accepted");
		require(!missing_name_terminator.contains(export_fixture::image_base + 0x180),
			"unterminated export name registered");

		export_list missing_library_terminator;
		require(!fixture.parse(missing_library_terminator, 96 + strlen(library)),
			"library name without an in-range terminator accepted");
	}

	void export_directory_and_array_bounds()
	{
		export_fixture fixture;
		export_list truncated_directory;
		require(!fixture.parse(truncated_directory, export_fixture::directory_offset + sizeof(IMAGE_EXPORT_DIRECTORY) - 1),
			"partial export directory accepted");
		export_list absent_directory;
		require(!absent_directory.add_exports(fixture.bytes.data(), fixture.bytes.size(), export_fixture::image_base, NULL, false),
			"null export directory accepted");
		for (SIZE_T length : {SIZE_T(82), SIZE_T(86), SIZE_T(89)})
		{
			export_list exports;
			require(!fixture.parse(exports, length), "partial export array accepted");
		}
		export_fixture large_functions;
		large_functions.directory()->NumberOfFunctions = MAXDWORD;
		export_list functions;
		require(!large_functions.parse(functions, large_functions.bytes.size()), "oversized export function count accepted");
		export_fixture large_names;
		large_names.directory()->NumberOfNames = MAXDWORD;
		export_list names;
		require(!large_names.parse(names, large_names.bytes.size()), "oversized export name count accepted");
		export_fixture ordinal_boundary;
		const WORD ordinal = 1;
		memcpy(ordinal_boundary.bytes.data() + 88, &ordinal, sizeof(ordinal));
		export_list ordinals;
		require(!ordinal_boundary.parse(ordinals, ordinal_boundary.bytes.size()), "ordinal equal to function count accepted");
	}

	void export_ordinal_lookup_and_miss()
	{
		export_list exports;
		char library[] = "ordinary.dll", name[] = "Example";
		export_entry ordinal_only(library, NULL, 1, 0x180, 0x400180, false);
		exports.add_export(ordinal_only.address, &ordinal_only);
		require(exports.find_export(library, name, false) == 0, "ordinal-only export matched a name");
		require(exports.find_export(NULL, name, false) == 0, "unfiltered lookup matched an ordinal-only export");
		require(exports.find_export(library, NULL, false) == 0, "null name lookup succeeded");
		export_entry found = exports.find(ordinal_only.address);
		require(found.name == NULL && found.ord == 1, "ordinal-only export not preserved");
		export_entry missing = exports.find(0x400181);
		require(missing.library_name == NULL && missing.name == NULL && missing.address == 0 && missing.rva == 0 &&
			missing.ord == 0 && !missing.is64, "missing export is not an empty value");
		export_entry empty(static_cast<export_entry*>(NULL));
		require(empty.library_name == NULL && empty.name == NULL && empty.address == 0, "null export copy is not empty");
	}

	void filename_sanitization()
	{
		std::string cleaned;
		require(dump_path::sanitize_label("Example-1.0_test", cleaned) && cleaned == "Example-1.0_test",
			"ordinary label changed");
		require(dump_path::sanitize_label("Example module+test", cleaned) && cleaned == "Example_module_test",
			"label punctuation not sanitized");
		for (unsigned int value = 1; value <= 255; ++value)
		{
			const char label[] = {static_cast<char>(value), 0};
			require(dump_path::sanitize_label(label, cleaned) && cleaned.size() == 1, "single-byte label rejected");
			const bool allowed = (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
				(value >= '0' && value <= '9') || value == '.' || value == '_' || value == '-';
			require(cleaned[0] == (allowed ? static_cast<char>(value) : '_'), "filename allowlist mismatch");
		}
		std::string filename, error;
		require(dump_path::make_filename(NULL, "sample_exe", 0x2a, "ordinary.dll", 0x401000, true, "dll", filename, error),
			"ordinary dump filename rejected");
		require(filename == "sample_exe_PID2a_ordinary.dll_401000_x64.dll", "dump filename convention changed");
		require(dump_path::make_filename("output", "sample_exe", 0x2a, "ordinary.dll", 0x401000, false, "exe", filename, error),
			"ordinary relative output root rejected");
		require(filename == "output\\sample_exe_PID2a_ordinary.dll_401000_x86.exe", "output root or architecture changed");
	}

	void filename_length_limits()
	{
		std::string filename = "unchanged", error, cleaned;
		const std::string long_label(256, 'a'), medium_label(125, 'b');
		require(!dump_path::sanitize_label(long_label.c_str(), cleaned), "overlong label accepted");
		require(!dump_path::make_filename(NULL, medium_label.c_str(), 1, medium_label.c_str(), 0x401000, false,
			"exe", filename, error) && filename.empty() && !error.empty(), "overlong combined component accepted");
		const std::string long_root(MAX_PATH, 'a');
		require(!dump_path::make_filename(long_root.c_str(), "sample", 1, "module", 0x401000, false,
			"exe", filename, error) && filename.empty() && !error.empty(), "overlong output path accepted");
		require(dump_path::is_device_component("CON.txt") && dump_path::is_device_component("lpt1.dll") &&
			!dump_path::is_device_component("ordinary.dll"), "reserved component classification changed");
	}

	void module_name_conversion()
	{
		const wchar_t source[] = L"caf\u00e9_\u4e2d.dll";
		std::string converted;
		require(module_names::to_ansi(source, _countof(source), converted, "test name"), "Unicode name conversion failed");
		const int expected = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
		require(expected > 0 && converted.size() + 1 == static_cast<SIZE_T>(expected), "Unicode conversion byte count incorrect");
		char short_buffer[4] = {1, 2, 3, 4};
		module_names::copy_wide(L"ordinary.dll", _countof(L"ordinary.dll"), short_buffer, 3, "test name");
		require(short_buffer[0] == 0 && short_buffer[3] == 4, "overlong converted name not safely rejected");
		const wchar_t unterminated[] = {L'a', L'b'};
		require(!module_names::to_ansi(unterminated, _countof(unterminated), converted, "test name") && converted.empty(),
			"unterminated wide name accepted");
		MODULEENTRY32 entry = {};
		wcscpy_s(entry.szModule, L"ordinary.dll");
		wcscpy_s(entry.szExePath, L"ordinary.dll");
		module item(entry);
		require(strcmp(item.short_name, "ordinary.dll") == 0 && strcmp(item.full_name, "ordinary.dll") == 0,
			"ordinary module name changed");
	}

	void module_name_api_failures()
	{
		char name[4] = {'a', 'b', 'c', 'd'};
		module_names::check_psapi_name(0, name, sizeof(name), "test name");
		require(name[0] == 0 && name[3] == 0, "failed module name read left uninitialized output");
		memset(name, 'a', sizeof(name));
		module_names::check_psapi_name(sizeof(name), name, sizeof(name), "test name");
		require(name[0] == 0 && name[3] == 0, "truncated module name was accepted");
	}

	void process_name_output_bounds()
	{
		test_options options;
		dump_process process(GetCurrentProcessId(), NULL, &options, true);
		char original[1024], repeated[1024];
		require(process.get_process_name(original, sizeof(original)), "self process name unavailable");
		char short_name[] = {'a', 'b'};
		require(!process.get_process_name(short_name, 1) && short_name[0] == 0 && short_name[1] == 'b',
			"short name output not safely cleared");
		require(process.get_process_name(repeated, sizeof(repeated)) && strcmp(original, repeated) == 0,
			"failed process name copy modified the stored name");
		require(!process.get_process_name(NULL, 1), "null process name output accepted");
		short_name[0] = 'a';
		require(!process.get_process_name(short_name, 0) && short_name[0] == 'a', "zero-capacity name output was modified");
	}

	void unhooked_monitor_stop()
	{
		test_options options;
		dump_process process(GetCurrentProcessId(), NULL, &options, true);
		require(process.monitor_close_stop(), "stopping an unhooked monitor failed");
		require(process.monitor_close_stop(), "stopping an unhooked monitor twice failed");
		require(!process.monitor_close_is_waiting(), "unhooked monitor reported a waiting callback");
		require(!process.monitor_close_dump_and_resume(), "unhooked monitor reported a successful resume");
	}
}

void append_name_safety_tests(std::vector<test_case>& tests)
{
	tests.push_back({"bounded export strings", export_bounded_strings});
	tests.push_back({"export directory and array bounds", export_directory_and_array_bounds});
	tests.push_back({"ordinal export lookup and misses", export_ordinal_lookup_and_miss});
	tests.push_back({"dump filename sanitization", filename_sanitization});
	tests.push_back({"dump filename length limits", filename_length_limits});
	tests.push_back({"module Unicode name conversion", module_name_conversion});
	tests.push_back({"module name API failures", module_name_api_failures});
	tests.push_back({"process name output bounds", process_name_output_bounds});
	tests.push_back({"unhooked monitor stop and destruction", unhooked_monitor_stop});
}
