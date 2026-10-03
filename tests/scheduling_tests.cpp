#include "test_support.h"
#include "system_work.h"
#include "module_work.h"
#include <atomic>
#include <chrono>
#include <set>

namespace
{
	void scheduler_lifetimes()
	{
		for (int threads : {1, 4})
		{
			work_pool pool(threads);
			std::atomic<int> completed(0), active(0), peak(0);
			for (int process = 0; process < 20; ++process)
				pool.submit([&] {
					std::vector<int> modules(513, 1);
					pool.parallel_for(modules, [&](int item) {
						const int running = ++active;
						int observed = peak.load();
						while (running > observed && !peak.compare_exchange_weak(observed, running)) {}
						completed += item;
						--active;
					});
					require(active >= 0, "module lifetime ended early");
				});
			pool.wait();
			require(completed == 20 * 513 && active == 0 && peak <= threads,
				"nested module work lost jobs, returned early or exceeded the worker limit");
			pool.submit([&] { ++completed; });
			pool.wait();
			require(completed == 20 * 513 + 1, "workers did not survive a drained queue");
		}
	}

	void scheduler_errors()
	{
		work_pool pool(2);
		std::atomic<int> completed(0);
		pool.submit([&] {
			std::vector<int> modules = {1, 2, 3, 4};
			pool.parallel_for(modules, [&](int item) {
				++completed;
				if (item == 2) throw std::runtime_error("expected module failure");
			});
		});
		bool reported = false;
		try { pool.wait(); }
		catch (const std::runtime_error& error) { reported = strcmp(error.what(), "expected module failure") == 0; }
		require(reported && completed == 4, "module error was hidden or siblings were not drained");
	}

	void final_process_uses_idle_workers()
	{
		work_pool pool(4);
		std::mutex mutex;
		std::condition_variable ready;
		int active = 0, peak = 0;
		pool.submit([&] {
			std::vector<int> modules(32);
			pool.parallel_for(modules, [&](int) {
				std::unique_lock<std::mutex> lock(mutex);
				peak = (std::max)(peak, ++active);
				ready.notify_all();
				const bool concurrent = ready.wait_for(lock, std::chrono::seconds(5), [&] { return peak >= 2; });
				--active;
				require(concurrent, "only one worker processed the final process's modules");
			});
		});
		pool.wait();
		require(peak >= 2 && peak <= 4 && active == 0, "tail module concurrency or lifetime is incorrect");
	}

	void final_process_scan()
	{
		work_pool pool(3);
		std::atomic<int> completed(0);
		int scans = 0;
		system_work(pool, true, [&] {
			if (++scans == 1) return std::vector<DWORD>{1, 2, 2};
			require(completed == 2, "final discovery ran before in-flight module work completed");
			return std::vector<DWORD>{2, 3};
		}, [&](DWORD pid, work_pool& workers) {
			std::vector<DWORD> modules{pid, pid};
			std::atomic<int> count(0);
			workers.parallel_for(modules, [&](DWORD) { ++count; });
			require(count == 2, "process finished before module jobs");
			++completed;
		});
		require(scans == 2 && completed == 3, "final pass missed new processes or repeated old PIDs");
		scans = 0;
		system_work(pool, false, [&] { ++scans; return std::vector<DWORD>{}; },
			[](DWORD, work_pool&) { throw std::runtime_error("empty inventory dispatched work"); });
		require(scans == 1, "hash generation unexpectedly rescanned processes");
	}

	void heap_boundaries()
	{
		std::set<unsigned __int64> heaps{0x1000, 0x2000, 0x3000, 0x4000};
		exclude_module_heaps(heaps, 0x2000, 0x2000);
		require(heaps == std::set<unsigned __int64>({0x1000, 0x4000}), "adjacent loose heap was removed");
	}

	void parallel_database_files()
	{
		temporary_file location, source;
		require(DeleteFileA(location.path) && CreateDirectoryA(location.path, NULL), "create database test folder");
		const std::string root(location.path), nested = root + "\\nested";
		std::vector<std::string> files;
		const auto cleanup = [&](void*) {
			for (const auto& path : files) DeleteFileA(path.c_str());
			RemoveDirectoryA(nested.c_str());
			RemoveDirectoryA(root.c_str());
		};
		std::unique_ptr<void, decltype(cleanup)> guard(reinterpret_cast<void*>(1), cleanup);
		require(CreateDirectoryA(nested.c_str(), NULL) != 0, "create nested database test folder");
		auto first = pe_fixture(false), second = pe_fixture(true);
		auto* second_header = reinterpret_cast<IMAGE_NT_HEADERS64*>(second.data() + 0x80);
		memcpy(reinterpret_cast<IMAGE_SECTION_HEADER*>(second_header + 1)->Name, ".other", 6);
		first[0x200] = second[0x200] = 0x31;
		first[0x201] = second[0x201] = 0xc0;
		for (int i = 0; i < 270; ++i)
		{
			source.write(first);
			const auto path = root + "\\" + std::to_string(i) + ".exe";
			require(CopyFileA(source.path, path.c_str(), TRUE) != 0, "copy database fixture");
			files.push_back(path);
		}
		source.write(second);
		for (const auto& path : {nested + "\\second.exe", root + "\\ignored.bin"})
		{
			require(CopyFileA(source.path, path.c_str(), TRUE) != 0, "copy nested database fixture");
			files.push_back(path);
		}
		test_options options;
		options.EntryPointHash = true;
		pe_header other(source.path, &options);
		require(other.process_pe_header() && other.process_sections(), "database reference parse");
		const auto second_hash = other.get_hash();
		for (bool recursive : {false, true})
		{
			temporary_file clean1, full1, short1, clean4, full4, short4;
			pe_hash_database serial(clean1.path, full1.path, short1.path), parallel(clean4.path, full4.path, short4.path);
			require(serial.add_folder(location.path, L"*.exe", recursive, 1) &&
				parallel.add_folder(location.path, L"*.exe", recursive, 4), "folder traversal failed");
			require(serial.count() == parallel.count() && serial.contains(second_hash) == recursive &&
				parallel.contains(second_hash) == recursive &&
				serial.snapshot_entrypoints()->full == parallel.snapshot_entrypoints()->full &&
				serial.snapshot_entrypoints()->short_hashes == parallel.snapshot_entrypoints()->short_hashes,
				"parallel traversal changed hashes, recursion or filename filtering");
		}
	}
}

void append_scheduling_tests(std::vector<test_case>& tests)
{
	tests.push_back({"module-scheduler-lifetimes", scheduler_lifetimes});
	tests.push_back({"module-scheduler-errors", scheduler_errors});
	tests.push_back({"system-tail-module-concurrency", final_process_uses_idle_workers});
	tests.push_back({"system-final-scan", final_process_scan});
	tests.push_back({"module-heap-boundaries", heap_boundaries});
	tests.push_back({"parallel-database-folders", parallel_database_files});
}
