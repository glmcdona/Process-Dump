#include "test_support.h"
#include "hash.h"
#include "dump_process.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <new>

namespace
{
	thread_local bool count_allocations = false;
	thread_local unsigned long long allocation_count = 0, allocation_bytes = 0;
}

#ifdef PD_PROFILE_ALLOCATIONS
void* operator new(size_t size)
{
	void* memory = malloc(size ? size : 1);
	if (memory == NULL)
		throw std::bad_alloc();
	if (count_allocations)
	{
		++allocation_count;
		allocation_bytes += size;
	}
	return memory;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { free(memory); }
void operator delete[](void* memory) noexcept { free(memory); }
void operator delete(void* memory, size_t) noexcept { free(memory); }
void operator delete[](void* memory, size_t) noexcept { free(memory); }
#endif

namespace
{
	long long ticks()
	{
		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		return value.QuadPart;
	}

	unsigned long long cpu_ticks()
	{
		FILETIME created, exited, kernel, user;
		require(GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user) != 0, "thread CPU timing failed");
		return (static_cast<unsigned long long>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
			(static_cast<unsigned long long>(user.dwHighDateTime) << 32) + user.dwLowDateTime;
	}

	unsigned long long cpu_cycles()
	{
		ULONG64 cycles = 0;
		require(QueryThreadCycleTime(GetCurrentThread(), &cycles) != 0, "thread cycle timing failed");
		return cycles;
	}

	struct result
	{
		long long capture = 0, reconstruct = 0, write = 0, cleanup = 0, total = 0;
		unsigned long long cpu = 0, cycles = 0, allocations = 0, allocated_bytes = 0;
		SIZE_T output_bytes = 0;
		DWORD crc = 0;
		std::unique_ptr<temporary_file> output;
	};

	void verify_output(result& measured)
	{
		if (!measured.output)
			return;
		auto bytes = measured.output->read();
		require(bytes.size() >= 0x80 + sizeof(IMAGE_NT_HEADERS64), "benchmark output header truncated");
		reinterpret_cast<IMAGE_NT_HEADERS64*>(bytes.data() + 0x80)->OptionalHeader.ImageBase = 0;
		measured.output_bytes = bytes.size();
		measured.crc = crc32buf(reinterpret_cast<char*>(bytes.data()), bytes.size());
	}

	class fixture
	{
	public:
		unsigned char* memory = NULL;
		SIZE_T size;
		explicit fixture(const std::string& workload)
		{
			const SIZE_T payload = workload == "sparse" ? 32 * 1024 * 1024 : 4 * 1024 * 1024;
			auto header = pe_fixture(true);
			size = 0x1000 + payload;
			memory = static_cast<unsigned char*>(VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
			require(memory != NULL, "benchmark allocation failed");
			memcpy(memory, header.data(), 0x200);
			auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(memory + 0x80);
			nt->OptionalHeader.SizeOfImage = static_cast<DWORD>(size);
			if (workload == "entrypoint" || workload == "entrypoint-empty")
				nt->OptionalHeader.AddressOfEntryPoint = 0;
			auto* section = reinterpret_cast<IMAGE_SECTION_HEADER*>(nt + 1);
			section->Misc.VirtualSize = static_cast<DWORD>(payload);
			section->SizeOfRawData = static_cast<DWORD>(payload);
			memset(memory + 0x1000, 0x90, workload == "sparse" ? 4096 : payload);
			if (workload == "imports")
				for (SIZE_T offset = 0x1000; offset + 8 <= size; offset += 64)
				{
					const unsigned __int64 address = 0x7fff12340000ULL + ((offset / 64) % 256) * 16;
					memcpy(memory + offset, &address, sizeof(address));
				}
			DWORD previous;
			if (VirtualProtect(memory, 0x1000, PAGE_READONLY, &previous) == 0)
			{
				VirtualFree(memory, 0, MEM_RELEASE);
				memory = NULL;
				throw std::runtime_error("benchmark header protection failed");
			}
		}
		~fixture() { if (memory != NULL) VirtualFree(memory, 0, MEM_RELEASE); }
		fixture(const fixture&) = delete;
		fixture& operator=(const fixture&) = delete;
	};

	std::unique_ptr<temporary_file> prepare_output(const std::string& workload)
	{
		std::unique_ptr<temporary_file> output;
		if (workload != "database" && workload != "exports")
		{
			output.reset(new temporary_file());
			require(DeleteFileA(output->path) != 0, "benchmark output reservation failed");
		}
		return output;
	}

	result measure(const std::string& workload, fixture& input, export_list& exports, pe_hash_database& database,
		bool verify, std::unique_ptr<temporary_file> output = {})
	{
		result measured;
		test_options options;
		options.ImportRec = workload == "imports";
		options.EntryPointHash = true;
		measured.output = output ? std::move(output) : prepare_output(workload);
		allocation_count = allocation_bytes = 0;
		const auto cpu_start = cpu_ticks();
		const auto cycles_start = cpu_cycles();
		const auto start = ticks();
		count_allocations = true;
		try
		{
			if (workload == "database")
			{
				unsigned int found = 0;
				for (unsigned int i = 0; i < 200000; ++i)
					found += database.contains(1 + i % 65536);
				require(found == 101696, "database benchmark result changed");
				measured.reconstruct = ticks() - start;
			}
			else if (workload == "exports")
			{
				dump_process process(GetCurrentProcessId(), &database, &options, true);
				require(process.build_export_list(), "export discovery benchmark failed");
				measured.reconstruct = ticks() - start;
			}
			else
			{
				pe_header image(GetCurrentProcess(), input.memory, NULL, &options);
				require(image.process_pe_header() && image.process_sections() && image.process_import_directory(),
					"benchmark capture failed");
				const auto captured = ticks();
				measured.capture = captured - start;
				require(image.process_disk_image(&exports, &database), "benchmark reconstruction failed");
				const auto reconstructed = ticks();
				measured.reconstruct = reconstructed - captured;
				require(image.write_image(measured.output->path), "benchmark write failed");
				measured.write = ticks() - reconstructed;
			}
			measured.total = ticks() - start;
			measured.cleanup = measured.total - measured.capture - measured.reconstruct - measured.write;
			count_allocations = false;
		}
		catch (...)
		{
			count_allocations = false;
			throw;
		}
		measured.cpu = cpu_ticks() - cpu_start;
		measured.cycles = cpu_cycles() - cycles_start;
		measured.allocations = allocation_count;
		measured.allocated_bytes = allocation_bytes;
		if (verify)
			verify_output(measured);
		return measured;
	}

	unsigned int argument(const char* text, unsigned int maximum)
	{
		char* end;
		const unsigned long value = strtoul(text, &end, 10);
		require(text[0] != 0 && *end == 0 && value > 0 && value <= maximum, "invalid benchmark count");
		return static_cast<unsigned int>(value);
	}
}

int run_performance_benchmark(int argc, char** argv)
{
	try
	{
		require(argc == 3, "usage: --benchmark dense|sparse|imports|entrypoint|entrypoint-empty|database|exports threads jobs");
		const std::string workload = argv[0];
		require(workload == "dense" || workload == "sparse" || workload == "imports" || workload == "entrypoint" ||
			workload == "entrypoint-empty" || workload == "database" || workload == "exports", "unknown benchmark workload");
		const unsigned int thread_count = argument(argv[1], 8), jobs = argument(argv[2], 256);
		require(jobs >= thread_count, "jobs must cover all benchmark workers");
		fixture input(workload);
		export_list exports;
		for (unsigned int i = 0; i < 256; ++i)
		{
			const auto address = 0x7fff12340000ULL + i * 16;
			const std::string name = "ExampleFunction" + std::to_string(i);
			export_entry entry("example.dll", name.c_str(), static_cast<WORD>(i + 1), 0x1000, address, true);
			exports.add_export(address, &entry);
		}
		temporary_file clean, ep, short_ep;
		pe_hash_database database(clean.path, ep.path, short_ep.path);
		if (workload == "database")
		{
			unordered_set<unsigned __int64> values;
			for (unsigned int i = 1; i <= 32768; ++i)
				values.insert(i);
			database.add_hashes(values);
		}
		if (workload == "entrypoint")
			database.add_hashes_eps({0x1234}, {0x1122334455667788ULL});
		const auto reference = measure(workload, input, exports, database, true);
		std::vector<result> results(jobs);
		for (auto& item : results)
			item.output = prepare_output(workload);
		std::vector<std::thread> workers;
		workers.reserve(thread_count);
		std::vector<long long> worker_finished(thread_count);
		std::atomic<unsigned int> next(0), ready(0);
		std::atomic<bool> start(false), failed(false);
		std::mutex error_lock;
		std::string error;
		try
		{
			for (unsigned int i = 0; i < thread_count; ++i)
				workers.emplace_back([&, i] {
					++ready;
					while (!start)
						SwitchToThread();
					try
					{
						for (unsigned int job; !failed && (job = next.fetch_add(1)) < jobs;)
							results[job] = measure(workload, input, exports, database, false, std::move(results[job].output));
					}
					catch (const std::exception& exception)
					{
						std::lock_guard<std::mutex> lock(error_lock);
						error = exception.what();
						failed = true;
					}
					worker_finished[i] = ticks();
				});
		}
		catch (...)
		{
			failed = true;
			start = true;
			for (auto& worker : workers)
				worker.join();
			throw;
		}
		while (ready != thread_count)
			SwitchToThread();
		const auto begin = ticks();
		start = true;
		for (auto& worker : workers)
			worker.join();
		const auto elapsed = ticks() - begin;
		const auto worker_elapsed = *std::max_element(worker_finished.begin(), worker_finished.end()) - begin;
		if (failed)
			throw std::runtime_error(error);
		PROCESS_MEMORY_COUNTERS memory = {};
		memory.cb = sizeof(memory);
		require(GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)) != 0, "memory counters failed");
		result sum;
		for (auto& item : results)
		{
			verify_output(item);
			require(reference.output_bytes == item.output_bytes && reference.crc == item.crc, "parallel output differs");
			sum.capture += item.capture;
			sum.reconstruct += item.reconstruct;
			sum.write += item.write;
			sum.cleanup += item.cleanup;
			sum.total += item.total;
			sum.cpu += item.cpu;
			sum.cycles += item.cycles;
			sum.allocations += item.allocations;
			sum.allocated_bytes += item.allocated_bytes;
		}
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		const double milliseconds = 1000.0 / frequency.QuadPart;
#ifdef PD_PROFILE_ALLOCATIONS
		const char* profiled = "true";
#else
		const char* profiled = "false";
#endif
		printf("PERF {\"workload\":\"%s\",\"threads\":%u,\"jobs\":%u,\"wall_ms\":%.3f,\"worker_ms\":%.3f,\"capture_ms\":%.3f,"
			"\"reconstruct_ms\":%.3f,\"write_ms\":%.3f,\"cleanup_ms\":%.3f,\"service_ms\":%.3f,\"cpu_ms\":%.3f,\"cpu_cycles\":%llu,"
			"\"profiled_allocations\":%s,\"allocations\":%llu,\"allocated_bytes\":%llu,"
			"\"peak_working_set\":%zu,\"output_bytes\":%zu,\"normalized_crc32\":\"%08lx\"}\n",
			workload.c_str(), thread_count, jobs, elapsed * milliseconds, worker_elapsed * milliseconds, sum.capture * milliseconds,
			sum.reconstruct * milliseconds, sum.write * milliseconds, sum.cleanup * milliseconds, sum.total * milliseconds,
			sum.cpu / 10000.0, sum.cycles, profiled, sum.allocations, sum.allocated_bytes, memory.PeakWorkingSetSize,
			reference.output_bytes, reference.crc);
		return 0;
	}
	catch (const std::exception& error)
	{
		fprintf(stderr, "BENCHMARK FAILED: %s\n", error.what());
		return 1;
	}
}
