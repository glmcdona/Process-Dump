#include "test_support.h"
#include "dump_process.h"
#include "dump_path.h"
#include <memory>
#include <atomic>
#include <thread>

namespace
{
	template<typename NT>
	NT& nt(std::vector<unsigned char>& data)
	{
		return *reinterpret_cast<NT*>(data.data() + 0x80);
	}

	class raw_pages
	{
	public:
		unsigned char* bytes;
		raw_pages()
		{
			bytes = static_cast<unsigned char*>(VirtualAlloc(NULL, 0x4000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
			require(bytes != NULL, "raw allocation failed");
			DWORD previous;
			require(VirtualProtect(bytes + 0x3000, 0x1000, PAGE_NOACCESS, &previous) != 0, "raw boundary setup failed");
			for (SIZE_T i = 0; i < 0x1000; ++i)
				bytes[0x2000 + i] = static_cast<unsigned char>(i % 251 + 1);
		}
		~raw_pages() { VirtualFree(bytes, 0, MEM_RELEASE); }
	};

	std::vector<unsigned char> write_reconstruction(pe_header& header)
	{
		temporary_file output;
		require(DeleteFileA(output.path) != 0, "output reservation failed");
		require(header.write_image(output.path), "reconstruction output failed");
		return output.read();
	}

	template<typename NT>
	void extended_optional_header(bool win64)
	{
		for (bool imports : {false, true})
		{
			auto bytes = pe_fixture(win64);
			auto& header = nt<NT>(bytes);
			auto* old_section = reinterpret_cast<unsigned char*>(&header + 1);
			memmove(old_section + 16, old_section, sizeof(IMAGE_SECTION_HEADER));
			memset(old_section, 0, 16);
			header.FileHeader.SizeOfOptionalHeader += 16;
			temporary_file input;
			input.write(bytes);
			test_options options;
			options.ImportRec = imports;
			pe_header image(input.path, &options);
			require(image.process_pe_header() && image.process_sections(), "extended optional header was not parsed");
			require(image.process_import_directory(), "extended-header imports rejected");
			export_list exports;
			require(image.process_disk_image(&exports, NULL), "extended-header reconstruction failed");
			auto dump = write_reconstruction(image);
			const auto* section = reinterpret_cast<IMAGE_SECTION_HEADER*>(dump.data() + 0x80 + sizeof(NT) + 16);
			require(section->VirtualAddress == 0x1000 && section->SizeOfRawData == 0x1000,
				"extended optional header changed the section layout");
			require(memcmp(dump.data() + section->PointerToRawData, bytes.data() + 0x200, 0x1000) == 0,
				"extended optional header lost section content");
		}
	}

	void export_discovery(DWORD function_rva, bool named, bool valid, bool forwarded, bool invalid_tail = false)
	{
		auto bytes = pe_fixture(true);
		nt<IMAGE_NT_HEADERS64>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] = {0x1800, 0x100};
		memset(bytes.data() + 0xa00, 0, 0x200);
		auto& directory = *reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(bytes.data() + 0xa00);
		directory.Name = 0x1960;
		directory.Base = 7;
		directory.NumberOfFunctions = invalid_tail ? 2 : 1;
		directory.NumberOfNames = named ? (invalid_tail ? 2 : 1) : 0;
		directory.AddressOfFunctions = 0x1900;
		directory.AddressOfNames = named ? 0x1940 : 0;
		directory.AddressOfNameOrdinals = named ? 0x1950 : 0;
		memcpy(bytes.data() + 0xb00, &function_rva, sizeof(function_rva));
		if (invalid_tail)
		{
			const DWORD outside = 0x3000;
			memcpy(bytes.data() + 0xb04, &outside, sizeof(outside));
		}
		const DWORD name = 0x1980;
		memcpy(bytes.data() + 0xb40, &name, sizeof(name));
		if (invalid_tail)
		{
			const DWORD invalid_name = 0x3000;
			const WORD second_ordinal = 1;
			memcpy(bytes.data() + 0xb44, &invalid_name, sizeof(invalid_name));
			memcpy(bytes.data() + 0xb52, &second_ordinal, sizeof(second_ordinal));
		}
		memcpy(bytes.data() + 0xb60, "example.dll", sizeof("example.dll"));
		memcpy(bytes.data() + 0xb80, "Example", sizeof("Example"));
		memcpy(bytes.data() + 0xa80, "other.Example", sizeof("other.Example"));
		temporary_file input;
		input.write(bytes);
		test_options options;
		pe_header image(input.path, &options);
		require(image.process_pe_header() && image.process_sections(), "export fixture rejected");
		require(image.process_export_directory() == valid, "invalid export result was hidden");
		if (!valid)
		{
			require(image.get_exports() == NULL, "invalid export table published a partial result");
			return;
		}
		require(image.get_exports()->contains(static_cast<unsigned __int64>(function_rva)) == !forwarded,
			"real export omitted or forwarder string treated as a callable address");
		if (!forwarded)
		{
			auto entry = image.get_exports()->find(function_rva);
			require(entry.ord == 7 && (entry.name != NULL) == named, "export identity lost");
		}
	}

	template<typename NT>
	void terminal_import_summary(bool win64)
	{
		auto bytes = pe_fixture(win64);
		const unsigned __int64 address = win64 ? 0x7fff12345678ULL : 0xf2345678ULL;
		const SIZE_T width = win64 ? 8 : 4;
		memcpy(bytes.data() + bytes.size() - width, &address, width);
		temporary_file input;
		input.write(bytes);
		test_options options;
		pe_header image(input.path, &options);
		require(image.process_pe_header() && image.process_sections(), "summary fixture rejected");
		export_list exports;
		export_entry entry("example.dll", "Example", 1, 0x1000, address, win64);
		exports.add_export(address, &entry);
		const auto summary = image.get_imports_information(&exports);
		require(summary.COUNT_UNIQUE_IMPORT_ADDRESSES == 1, "summary omitted a terminal import pointer");
		require(summary.COUNT_UNIQUE_IMPORT_LIBRARIES == 1, "summary omitted the import library count");
		require(image.get_imports_information(&exports, 0x2000 - width).COUNT_UNIQUE_IMPORT_ADDRESSES == 0,
			"summary read beyond its requested size limit");
		unsigned __int64 ordinal_hashes[2];
		for (WORD ordinal = 7; ordinal <= 8; ++ordinal)
		{
			export_list ordinal_exports;
			export_entry ordinal_entry("example.dll", NULL, ordinal, 0x1000, address, win64);
			ordinal_exports.add_export(address, &ordinal_entry);
			ordinal_hashes[ordinal - 7] = image.get_imports_information(&ordinal_exports).HASH_GENERIC;
			pe_imports imports(NULL, 0, NULL, win64);
			imports.add_fixup(ordinal_entry.library_name, ordinal_entry.ord, 0x1000, win64);
			__int64 descriptors = 0, extra = 0;
			imports.get_table_size(descriptors, extra);
			std::vector<unsigned char> table(static_cast<SIZE_T>(descriptors + extra));
			require(imports.build_table(table.data(), table.size(), 0x3000, 0, descriptors), "ordinal import table failed");
			const auto& descriptor = *reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(table.data());
			unsigned __int64 thunk = 0;
			memcpy(&thunk, table.data() + descriptor.OriginalFirstThunk - 0x3000, width);
			require(thunk == ((win64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) | ordinal),
				"ordinal import encoding lost its identity");
		}
		require(ordinal_hashes[0] != ordinal_hashes[1], "different ordinal imports have the same generic identity");
	}

	void generated_image_type(bool win64)
	{
		raw_pages memory;
		for (bool dll : {false, true})
		{
			test_options options;
			options.ReconstructHeaderAsDll = dll;
			pe_header image(GetCurrentProcess(), memory.bytes + 0x2000, NULL, &options);
			require(image.build_pe_header(0x1000, win64, 1) && image.process_sections(), "generated image failed");
			require(image.is_dll() == dll && image.is_exe() != dll, "generated image type flag is reversed");
			require(image.process_import_directory() && image.process_disk_image(NULL, NULL), "raw image reconstruction failed");
			auto dump = write_reconstruction(image);
			const auto* section = reinterpret_cast<const IMAGE_SECTION_HEADER*>(dump.data() + 0xe0 +
				(win64 ? sizeof(IMAGE_NT_HEADERS64) : sizeof(IMAGE_NT_HEADERS32)));
			require(section->SizeOfRawData == 0x1000 &&
				memcmp(dump.data() + section->PointerToRawData, memory.bytes + 0x2000, 0x1000) == 0,
				"generated image lost raw bytes");
		}
	}

	void raw_address_dump()
	{
		raw_pages memory;
		temporary_file clean, ep, short_ep, directory;
		require(DeleteFileA(directory.path) && CreateDirectoryA(directory.path, NULL), "raw output directory failed");
		std::vector<std::string> paths;
		auto cleanup = [&paths](char* path) {
			for (const auto& file : paths)
				DeleteFileA(file.c_str());
			RemoveDirectoryA(path);
		};
		std::unique_ptr<char, decltype(cleanup)> output(directory.path, cleanup);
		pe_hash_database database(clean.path, ep.path, short_ep.path);
		test_options options;
		options.set_output_path(directory.path);
		dump_process dumper(GetCurrentProcessId(), &database, &options, true);
		char process_name[1024];
		require(dumper.get_process_name(process_name, sizeof(process_name)), "self process name missing");
		for (bool win64 : {false, true})
		{
			std::string file, error;
			require(dump_path::make_filename(directory.path, process_name, GetCurrentProcessId(), "hiddenmodule",
				reinterpret_cast<uintptr_t>(memory.bytes + 0x2000), win64, "exe", file, error), "raw filename failed");
			paths.push_back(file);
		}
		dumper.dump_region(reinterpret_cast<uintptr_t>(memory.bytes + 0x2000));
		for (const auto& file : paths)
		{
			require(GetFileAttributesA(file.c_str()) != INVALID_FILE_ATTRIBUTES, "address-specific raw dumping produced no image");
			test_options check_options;
			pe_header check(const_cast<char*>(file.c_str()), &check_options);
			require(check.process_pe_header() && check.process_sections(), "raw output is not a readable PE");
		}
	}

	BOOL WINAPI many_modules(HANDLE, HMODULE* modules, DWORD capacity, LPDWORD needed, DWORD)
	{
		const DWORD count = 4097;
		*needed = count * sizeof(HMODULE);
		for (DWORD i = 0; i < (std::min<DWORD>)(count, capacity / sizeof(HMODULE)); ++i)
			modules[i] = reinterpret_cast<HMODULE>(static_cast<uintptr_t>(i + 1));
		return TRUE;
	}

	BOOL WINAPI growing_modules(HANDLE, HMODULE*, DWORD capacity, LPDWORD needed, DWORD)
	{
		*needed = capacity + sizeof(HMODULE);
		return TRUE;
	}

	BOOL WINAPI unavailable_modules(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD)
	{
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}

	void module_snapshot_bounds()
	{
		std::vector<HMODULE> modules;
		require(module_snapshot::read(NULL, modules, many_modules), "large module snapshot failed");
		require(modules.size() == 4097, "large module list was truncated");
		for (SIZE_T i = 0; i < modules.size(); ++i)
			require(reinterpret_cast<uintptr_t>(modules[i]) == i + 1, "unfilled module slot exposed");
		require(!module_snapshot::read(NULL, modules, growing_modules) && modules.empty(),
			"perpetually growing module snapshot was accepted");
		require(!module_snapshot::read(NULL, modules, unavailable_modules) && modules.empty() &&
			GetLastError() == ERROR_ACCESS_DENIED, "module enumeration error was hidden");
	}

	void default_dump_options()
	{
		PD_OPTIONS options;
		require(options.ImportRec && !options.ForceGenHeader && !options.Verbose && !options.ReconstructHeaderAsDll &&
			options.DumpChunks && options.EntryPointHash && !options.ForceReconstructEntryPoint &&
			!options.Reexecution && options.NumberOfThreads == 16 && options.EntryPointOverride == -1 && options.output_path[0] == 0,
			"default dump/database options are not initialized consistently");
	}

	void concurrent_database_lookup()
	{
		temporary_file clean, ep, short_ep;
		pe_hash_database database(clean.path, ep.path, short_ep.path);
		std::atomic<bool> valid(true);
		std::vector<std::thread> workers;
		for (unsigned int worker = 0; worker < 4; ++worker)
			workers.emplace_back([&, worker] {
				for (unsigned int batch = 0; batch < 300; ++batch)
				{
					unordered_set<unsigned __int64> values;
					for (unsigned int i = 0; i < 64; ++i)
						values.insert(1 + worker * 19200 + batch * 64 + i);
					database.add_hashes(values);
					database.add_hashes_eps(values, values);
					for (auto value : values)
						if (!database.contains(value) || !database.contains_ep(value) || !database.contains_epshort(value))
							valid = false;
				}
			});
		for (auto& worker : workers)
			worker.join();
		require(valid && database.count() == 76800 && database.count_eps() == 76800 &&
			database.count_epshorts() == 76800, "concurrent database updates lost hashes");
	}
}

void append_pipeline_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe32-extended-optional-header", [] { extended_optional_header<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-extended-optional-header", [] { extended_optional_header<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"page-aligned-export", [] { export_discovery(0x1000, true, true, false); }});
	tests.push_back({"ordinal-only-export", [] { export_discovery(0x1100, false, true, false); }});
	tests.push_back({"forwarded-export-is-not-code", [] { export_discovery(0x1880, true, true, true); }});
	tests.push_back({"redirected-export-preserved", [] { export_discovery(0x3000, true, true, false); }});
	tests.push_back({"export-hole-is-not-an-address", [] { export_discovery(0, true, true, true); }});
	tests.push_back({"invalid-export-table-is-atomic", [] { export_discovery(0x1000, true, false, false, true); }});
	tests.push_back({"pe32-terminal-import-summary", [] { terminal_import_summary<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-terminal-import-summary", [] { terminal_import_summary<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-generated-image-type", [] { generated_image_type(false); }});
	tests.push_back({"pe64-generated-image-type", [] { generated_image_type(true); }});
	tests.push_back({"raw-address-dump", raw_address_dump});
	tests.push_back({"large-and-changing-module-snapshots", module_snapshot_bounds});
	tests.push_back({"default-dump-options", default_dump_options});
	tests.push_back({"concurrent-database-lookup", concurrent_database_lookup});
}
