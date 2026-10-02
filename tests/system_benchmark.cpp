#include "test_support.h"
#include "dump_process.h"
#include "system_work.h"
#include <chrono>
#include <fstream>
#include <set>

int run_system_fixture()
{
	try
	{
		std::vector<std::unique_ptr<void, std::function<void(void*)>>> images;
		for (int i = 0; i < 64; ++i)
		{
			auto bytes = pe_fixture(sizeof(void*) == 8);
			void* memory = VirtualAlloc(NULL, 0x202000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			require(memory != NULL, "allocate inert system fixture");
			images.emplace_back(memory, [](void* block) { VirtualFree(block, 0, MEM_RELEASE); });
			auto* base = static_cast<unsigned char*>(memory);
			memcpy(base, bytes.data(), 0x200);
			memcpy(base + 0x1000, bytes.data() + 0x200, 0x1000);
			auto* header = reinterpret_cast<IMAGE_NT_HEADERS*>(base + 0x80);
			header->OptionalHeader.ImageBase = reinterpret_cast<uintptr_t>(memory);
			header->OptionalHeader.SizeOfImage = 0x202000;
			auto* section = reinterpret_cast<IMAGE_SECTION_HEADER*>(header + 1);
			sprintf_s(reinterpret_cast<char*>(section->Name), 8, "mod%03d", i);
			section->Misc.VirtualSize = 0x201000;
		}
		puts("PD_SYSTEM_READY");
		fflush(stdout);
		Sleep(600000);
		return 0;
	}
	catch (const std::exception& error)
	{
		fprintf(stderr, "System fixture failed: %s\n", error.what());
		return 1;
	}
}

int run_system_benchmark(int argc, char** argv)
{
	try
	{
		require(argc == 6, "usage: --system-work pid dump|hash threads output report flags(i,c,g,r)");
		const DWORD pid = strtoul(argv[0], NULL, 10);
		const int threads = atoi(argv[2]);
		require(pid != 0 && threads >= 0 && threads <= 64, "invalid process or worker count");
		test_options options;
		options.EntryPointHash = true;
		options.ImportRec = strchr(argv[5], 'i') != NULL;
		options.DumpChunks = strchr(argv[5], 'c') != NULL;
		options.ForceGenHeader = strchr(argv[5], 'g') != NULL;
		options.Reexecution = strchr(argv[5], 'r') != NULL;
		delete[] options.output_path;
		options.output_path = new char[strlen(argv[3]) + 1];
		strcpy(options.output_path, argv[3]);
		temporary_file clean, ep, prefix;
		pe_hash_database database(clean.path, ep.path, prefix.path);
		const bool hashing = strcmp(argv[1], "hash") == 0;
		require(hashing || strcmp(argv[1], "dump") == 0, "invalid system workload");
		unordered_set<unsigned __int64> hashes, full, prefixes;
		const auto action = [&](work_pool* pool) {
			dump_process process(pid, &database, &options, true);
			if (!hashing) process.dump_all(pool);
			process.get_all_hashes(&hashes, hashing ? &full : NULL, hashing ? &prefixes : NULL, pool);
			database.add_hashes(hashes);
			if (hashing) database.add_hashes_eps(full, prefixes);
		};
		const auto started = std::chrono::steady_clock::now();
		if (threads)
		{
			work_pool pool(threads);
			system_work(pool, !hashing, [=] { return std::vector<DWORD>{pid}; },
				[&](DWORD, work_pool& workers) { action(&workers); });
		}
		else action(NULL);
		const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
		std::ofstream output(argv[4]);
		require(output.good(), "cannot open system benchmark report");
		output << "{\"milliseconds\":" << ms << ",\"hashes\":[";
		const auto write = [&](const unordered_set<unsigned __int64>& values) {
			bool first = true;
			for (auto value : std::set<unsigned __int64>(values.begin(), values.end()))
			{
				if (!first) output << ',';
				first = false;
				output << value;
			}
		};
		write(hashes); output << "],\"full\":[";
		write(full); output << "],\"prefixes\":[";
		write(prefixes); output << "]}\n";
		require(output.good(), "cannot write system benchmark report");
		return 0;
	}
	catch (const std::exception& error)
	{
		fprintf(stderr, "System benchmark failed: %s\n", error.what());
		return 1;
	}
}
