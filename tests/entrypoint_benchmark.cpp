#include "test_support.h"
#include "pe_hash_database.h"
#include <fstream>
#include <map>
#include <set>
#include <chrono>

struct entrypoint_benchmark
{
	struct sample
	{
		std::string path;
		DWORD truth;
		unsigned __int64 full, prefix;
	};

	static bool executable(pe_header& image, SIZE_T rva, SIZE_T length = 1)
	{
		for (int i = 0; i < image._num_sections; ++i)
		{
			const auto& section = image._header_sections[i];
			if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) && rva >= section.VirtualAddress &&
				range_fits((std::max)(section.Misc.VirtualSize, section.SizeOfRawData), rva - section.VirtualAddress, length))
				return true;
		}
		return false;
	}

	static DWORD unique(const std::set<DWORD>& candidates)
	{
		return candidates.size() == 1 ? *candidates.begin() : 0;
	}

	static int run(const char* manifest, const char* report)
	{
		test_options options;
		options.EntryPointHash = true;
		std::ifstream input(manifest);
		require(input.good(), "cannot read entrypoint corpus manifest");
		std::vector<sample> samples;
		std::map<unsigned __int64, int> full_counts, prefix_counts;
		std::string path;
		while (std::getline(input, path))
		{
			if (!path.empty() && path.back() == '\r') path.pop_back();
			pe_header image(&path[0], &options);
			if (!image.process_pe_header() || !image.process_sections()) continue;
			const DWORD truth = image._parsed_pe_32 ? image._header_pe32->OptionalHeader.AddressOfEntryPoint :
				image._header_pe64->OptionalHeader.AddressOfEntryPoint;
			if (!truth || !executable(image, truth)) continue;
			const auto full = image.get_hash_ep(), prefix = image.get_hash_ep_short();
			samples.push_back({path, truth, full, prefix});
			if (full) ++full_counts[full];
			if (prefix) ++prefix_counts[prefix];
		}
		std::ofstream output(report);
		require(output.good(), "cannot create entrypoint corpus report");
		output << "index,architecture,truth,heldout,legacy,code_first,unique_full,boundary_fallback,weak_unique,full_candidates,fallback_candidates,prefer_exact,runtime_only,actual,ms,path\n";
		for (size_t index = 0; index < samples.size(); ++index)
		{
			auto& source = samples[index];
			pe_header image(&source.path[0], &options);
			require(image.process_pe_header() && image.process_sections(), "corpus changed during measurement");
			for (bool heldout : {false, true})
			{
				const auto started = std::chrono::steady_clock::now();
				const auto known_full = [&](unsigned __int64 hash) {
					auto found = full_counts.find(hash);
					return hash && found != full_counts.end() && found->second > (heldout && hash == source.full ? 1 : 0);
				};
				const auto known_prefix = [&](unsigned __int64 hash) {
					auto found = prefix_counts.find(hash);
					return hash && found != prefix_counts.end() && found->second > (heldout && hash == source.prefix ? 1 : 0);
				};
				DWORD first = 0, strong = 0, first_code = 0;
				std::set<DWORD> full, weak, fallback, boundaries, runtime;
				for (SIZE_T rva = 0; range_fits(image._image_size, rva, 8); ++rva)
				{
					unsigned __int64 prefix = 0;
					memcpy(&prefix, image._image + rva, 8);
					if (known_prefix(prefix))
					{
						const bool matched = known_full(image._hash_asm(rva));
						if (rva >= 0x1000 && rva < image._image_size - 8)
						{
							if (!first) first = static_cast<DWORD>(rva);
							if (matched && !strong) strong = static_cast<DWORD>(rva);
						}
						if (executable(image, rva, 8))
						{
							weak.insert(static_cast<DWORD>(rva));
							if (matched)
							{
								full.insert(static_cast<DWORD>(rva));
								if (!first_code) first_code = static_cast<DWORD>(rva);
							}
						}
					}
					if (executable(image, rva, 5) && image._image[rva] == 0xe8)
					{
						INT32 displacement = 0;
						memcpy(&displacement, image._image + rva + 1, 4);
						const auto target = static_cast<__int64>(rva) + 5 + displacement;
						if (target > 0 && target < static_cast<__int64>(image._image_size) && executable(image, target, 20))
							boundaries.insert(static_cast<DWORD>(target));
					}
				}
				if (image._parsed_pe_64)
				{
					const auto& directory = image._header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
					if (range_fits(image._image_size, directory.VirtualAddress, directory.Size))
						for (SIZE_T i = 0; range_fits(directory.Size, i, sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)); i += sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY))
						{
							IMAGE_RUNTIME_FUNCTION_ENTRY function;
							memcpy(&function, image._image + directory.VirtualAddress + i, sizeof(function));
							if (function.BeginAddress < function.EndAddress && executable(image, function.BeginAddress, 20))
							{
								boundaries.insert(function.BeginAddress);
								if (known_full(image._hash_asm(function.BeginAddress)))
									runtime.insert(function.BeginAddress);
							}
						}
				}
				fallback = full;
				for (DWORD rva : boundaries)
					if (known_full(image._hash_asm(rva))) fallback.insert(rva);
				temporary_file clean, ep, short_ep;
				pe_hash_database database(clean.path, ep.path, short_ep.path);
				unordered_set<unsigned __int64> full_set, prefix_set;
				for (const auto& item : full_counts) if (known_full(item.first)) full_set.insert(item.first);
				for (const auto& item : prefix_counts) if (known_prefix(item.first)) prefix_set.insert(item.first);
				database.add_hashes_eps(full_set, prefix_set);
				DWORD& entry = image._parsed_pe_32 ? image._header_pe32->OptionalHeader.AddressOfEntryPoint :
					image._header_pe64->OptionalHeader.AddressOfEntryPoint;
				entry = 0;
				image._recover_entrypoint(&database);
				const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
				output << index << ',' << (image.is_64() ? 64 : 32) << ',' << source.truth << ',' << heldout << ','
					<< (strong ? strong : first) << ',' << first_code << ',' << unique(full) << ',' << unique(fallback) << ','
					<< (unique(full) ? unique(full) : unique(weak)) << ',' << full.size() << ',' << fallback.size() << ','
					<< (first_code ? first_code : unique(fallback)) << ',' << (first_code ? first_code : unique(runtime))
					<< ',' << entry << ',' << ms << ",\"" << source.path << "\"\n";
				output.flush();
			}
		}
		require(output.good(), "entrypoint report write failed");
		printf("Measured %zu corpus images.\n", samples.size());
		return 0;
	}

	static void recovery_rules(bool win64)
	{
		test_options options;
		options.EntryPointHash = true;
		auto bytes = pe_fixture(win64);
		memset(bytes.data() + 0x200, 0xcc, 0x1000);
		auto* code = bytes.data() + 0x240;
		memset(code, 0x90, 128);
		code[0] = 0xe8;
		code[1] = 0x34; code[2] = 0x12; code[3] = code[4] = 0;
		code[40] = 0xc3;
		temporary_file file, clean, ep, prefix;
		file.write(bytes);
		pe_hash_database database(clean.path, ep.path, prefix.path);
		pe_header image(file.path, &options);
		require(image.process_pe_header() && image.process_sections(), "recovery rules parse failed");
		const auto full = image._hash_asm(0x1040), short_hash = image._hash_short_asm(0x1040);
		require(full && short_hash, "recovery rules signatures missing");
		database.add_hashes_eps({full}, {short_hash});
		DWORD& entry = win64 ? image._header_pe64->OptionalHeader.AddressOfEntryPoint :
			image._header_pe32->OptionalHeader.AddressOfEntryPoint;
		entry = 0;
		image._recover_entrypoint(&database);
		require(entry == 0x1040, "exact recovery failed");
		entry = 0x1000;
		image._recover_entrypoint(&database);
		require(entry == 0x1000, "valid entrypoint was overwritten");
		options.ForceReconstructEntryPoint = true;
		options.Verbose = true;
		image._recover_entrypoint(&database);
		require(entry == 0x1040, "forced/verbose recovery changed selection");
		options.EntryPointHash = false;
		entry = 0;
		image._recover_entrypoint(&database);
		require(entry == 0, "disabled recovery ran");
		options.EntryPointHash = true;
		image._header_sections[0].Characteristics &= ~IMAGE_SCN_MEM_EXECUTE;
		image._recover_entrypoint(&database);
		require(entry == 0, "nonexecutable signature was selected");
		image._header_sections[0].Characteristics |= IMAGE_SCN_MEM_EXECUTE;
		if (win64)
		{
			auto& directory = image._header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
			directory = {0x1e00, sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)};
			IMAGE_RUNTIME_FUNCTION_ENTRY function = {0x1040, 0x1080, 0x1f00};
			memcpy(image._image + 0x1e00, &function, sizeof(function));
			image._image[0x1041] ^= 0x10;
			image._recover_entrypoint(&database);
			require(entry == 0x1040, "runtime metadata did not recover a changed raw prefix");
			entry = 0;
			function.EndAddress = 0x4000;
			memcpy(image._image + 0x1e00, &function, sizeof(function));
			image._recover_entrypoint(&database);
			require(entry == 0, "invalid runtime extent was accepted");
			function.EndAddress = 0x1080;
			memcpy(image._image + 0x1e00, &function, sizeof(function));
			memcpy(image._image + 0x1100, image._image + 0x1040, 128);
			function = {0x1100, 0x1140, 0x1f00};
			memcpy(image._image + 0x1e00 + sizeof(function), &function, sizeof(function));
			directory.Size *= 2;
			image._recover_entrypoint(&database);
			require(entry == 0, "ambiguous metadata fallback guessed an entrypoint");
			directory.Size = sizeof(function) - 1;
			image._recover_entrypoint(&database);
			require(entry == 0, "partial runtime record was accepted");
		}
		options.ForceReconstructEntryPoint = false;
		options.Verbose = false;
		for (DWORD start : {0x200u, 0x2000u})
		{
			auto shifted = bytes;
			IMAGE_SECTION_HEADER* section = NULL;
			if (win64)
			{
				auto* header = reinterpret_cast<IMAGE_NT_HEADERS64*>(shifted.data() + 0x80);
				header->OptionalHeader.AddressOfEntryPoint = start;
				header->OptionalHeader.SectionAlignment = start == 0x200 ? 0x200 : 0x1000;
				header->OptionalHeader.SizeOfImage = start + 0x1000;
				section = reinterpret_cast<IMAGE_SECTION_HEADER*>(header + 1);
			}
			else
			{
				auto* header = reinterpret_cast<IMAGE_NT_HEADERS32*>(shifted.data() + 0x80);
				header->OptionalHeader.AddressOfEntryPoint = start;
				header->OptionalHeader.SectionAlignment = start == 0x200 ? 0x200 : 0x1000;
				header->OptionalHeader.SizeOfImage = start + 0x1000;
				section = reinterpret_cast<IMAGE_SECTION_HEADER*>(header + 1);
			}
			section->VirtualAddress = start;
			file.write(shifted);
			pe_header low(file.path, &options);
			require(low.process_pe_header() && low.process_sections(), "shifted entrypoint fixture parse failed");
			DWORD& found = win64 ? low._header_pe64->OptionalHeader.AddressOfEntryPoint :
				low._header_pe32->OptionalHeader.AddressOfEntryPoint;
			low._recover_entrypoint(&database);
			require(found == start, "valid low-alignment or RVA 0x2000 entrypoint was overwritten");
			found = 0;
			low._recover_entrypoint(&database);
			require(found == start + 0x40, "executable scan missed low-alignment entrypoint");
		}
	}
};

void append_entrypoint_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe32-entrypoint-selection-rules", [] { entrypoint_benchmark::recovery_rules(false); }});
	tests.push_back({"pe64-entrypoint-selection-rules", [] { entrypoint_benchmark::recovery_rules(true); }});
}

int run_entrypoint_benchmark(int argc, char** argv)
{
	try
	{
		require(argc == 2, "usage: --entrypoint-corpus manifest.txt report.csv");
		return entrypoint_benchmark::run(argv[0], argv[1]);
	}
	catch (const std::exception& error)
	{
		fprintf(stderr, "Entrypoint benchmark failed: %s\n", error.what());
		return 1;
	}
}
