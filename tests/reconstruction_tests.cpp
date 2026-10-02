#include "test_support.h"

namespace
{
	template<typename NT>
	NT& nt(std::vector<unsigned char>& bytes)
	{
		return *reinterpret_cast<NT*>(bytes.data() + 0x80);
	}

	template<typename NT>
	IMAGE_SECTION_HEADER* sections(std::vector<unsigned char>& bytes)
	{
		return reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt<NT>(bytes) + 1);
	}

	std::vector<unsigned char> mapped_fixture(bool win64, SIZE_T size)
	{
		const auto file = pe_fixture(win64);
		std::vector<unsigned char> image(size);
		memcpy(image.data(), file.data(), 0x200);
		memcpy(image.data() + 0x1000, file.data() + 0x200, 0x1000);
		return image;
	}

	std::vector<unsigned char> reconstruct(std::vector<unsigned char>& image, bool imports,
		unsigned __int64 export_address = 0x12345678)
	{
		test_options options;
		options.ImportRec = imports;
		pe_header header(GetCurrentProcess(), image.data(), NULL, &options);
		require(header.process_pe_header() && header.process_sections(), "mapped fixture rejected");
		require(header.process_import_directory(), "fixture imports rejected");
		export_list exports;
		char library[] = "example.dll", function[] = "ExampleFunction";
		export_entry entry(library, function, 1, 0x1000, export_address, header.is_64());
		exports.add_export(export_address, &entry);
		require(header.process_disk_image(&exports, NULL), "reconstruction failed");
		temporary_file output;
		require(DeleteFileA(output.path) != 0, "could not reserve output path");
		require(header.write_image(output.path), "reconstruction write failed");
		return output.read();
	}

	template<typename NT>
	SIZE_T file_offset(std::vector<unsigned char>& bytes, DWORD rva)
	{
		auto& header = nt<NT>(bytes);
		for (WORD i = 0; i < header.FileHeader.NumberOfSections; ++i)
		{
			const auto& section = sections<NT>(bytes)[i];
			if (rva >= section.VirtualAddress && rva - section.VirtualAddress < section.SizeOfRawData)
			{
				const SIZE_T offset = section.PointerToRawData + rva - section.VirtualAddress;
				require(offset < bytes.size(), "RVA maps outside the dump");
				return offset;
			}
		}
		throw std::runtime_error("RVA is not backed by section data");
	}

	template<typename NT>
	void sparse_data(bool win64, bool imports)
	{
		auto image = mapped_fixture(win64, 0x402000);
		auto& header = nt<NT>(image);
		header.FileHeader.NumberOfSections = 2;
		header.OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
		auto& data = sections<NT>(image)[1];
		memcpy(data.Name, ".bss", 4);
		data.VirtualAddress = 0x2000;
		data.Misc.VirtualSize = 0x400000;
		data.Characteristics = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		image[0x2123] = 0x5a; // Preserve initialized runtime data in an otherwise zero-filled section.
		auto dump = reconstruct(image, imports);
		require(dump.size() <= (imports ? 0x4000u : 0x3000u), "zero-filled virtual tail expanded on disk");
		require(sections<NT>(dump)[1].Misc.VirtualSize == 0x400000, "virtual allocation was truncated");
		require((sections<NT>(dump)[1].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) != 0,
			"initialized runtime data is still marked uninitialized");
		require(dump[file_offset<NT>(dump, 0x2123)] == 0x5a, "runtime-initialized data was lost");
		require(nt<NT>(dump).OptionalHeader.SizeOfImage >= image.size(), "virtual image size was replaced by disk size");
	}

	template<typename NT>
	void sparse_rvas(bool win64)
	{
		auto image = mapped_fixture(win64, 0x9000);
		auto& header = nt<NT>(image);
		header.FileHeader.NumberOfSections = 2;
		header.OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
		auto& data = sections<NT>(image)[1];
		memcpy(data.Name, ".data", 5);
		data.VirtualAddress = 0x8000;
		data.Misc.VirtualSize = data.SizeOfRawData = 0x1000;
		data.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		image[0x8fff] = 0x5a;
		auto dump = reconstruct(image, false);
		require(dump.size() == 0x3000, "RVA gap was materialized on disk");
		require(nt<NT>(dump).OptionalHeader.SizeOfImage == 0x9000, "SizeOfImage does not cover section RVAs");
		require(dump[file_offset<NT>(dump, 0x8fff)] == 0x5a, "sparse section content lost");
	}

	template<typename NT>
	void unaligned_imports(bool win64)
	{
		auto image = mapped_fixture(win64, 0x2801);
		auto& header = nt<NT>(image);
		header.OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
		sections<NT>(image)[0].Misc.VirtualSize = 0x1801;
		image.back() = 0x5a;
		const unsigned __int64 address = 0x12345678;
		memcpy(image.data() + 0x1020, &address, win64 ? 8 : 4);
		auto dump = reconstruct(image, true);
		const auto directory = nt<NT>(dump).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		const SIZE_T offset = file_offset<NT>(dump, directory.VirtualAddress);
		require(range_fits(dump.size(), offset, sizeof(IMAGE_IMPORT_DESCRIPTOR)), "import descriptor outside dump");
		const auto& descriptor = *reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(dump.data() + offset);
		const SIZE_T name = file_offset<NT>(dump, descriptor.Name);
		require(range_fits(dump.size(), name, sizeof("example.dll")), "import name outside dump");
		require(memcmp(dump.data() + name, "example.dll", sizeof("example.dll")) == 0,
			"import name RVA uses the unaligned image end");
	}

	template<typename NT>
	void large_section(bool win64)
	{
		const DWORD size = 60 * 1000 * 1024 + 0x2000;
		auto image = mapped_fixture(win64, size + 0x1000);
		nt<NT>(image).OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
		auto& section = sections<NT>(image)[0];
		section.Misc.VirtualSize = size;
		image.back() = 0x5a;
		auto dump = reconstruct(image, false);
		require(sections<NT>(dump)[0].Misc.VirtualSize == size, "valid large section was arbitrarily truncated");
		require(dump[file_offset<NT>(dump, static_cast<DWORD>(image.size() - 1))] == 0x5a,
			"large section tail lost");
	}

	template<typename NT>
	void full_header(bool win64)
	{
		auto image = mapped_fixture(win64, 0x5000);
		nt<NT>(image).OptionalHeader.SizeOfImage = static_cast<DWORD>(image.size());
		// Keep header-resident data where an extra section header would otherwise go.
		unsigned char* reserved = reinterpret_cast<unsigned char*>(sections<NT>(image) + 1);
		memset(reserved, 0x5a, sizeof(IMAGE_SECTION_HEADER));
		auto dump = reconstruct(image, true);
		require(nt<NT>(dump).FileHeader.NumberOfSections == 1, "header data overwritten by import section");
		require(memcmp(reinterpret_cast<unsigned char*>(sections<NT>(dump) + 1), reserved,
			sizeof(IMAGE_SECTION_HEADER)) == 0, "header-resident data lost");
		const auto directory = nt<NT>(dump).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		require(directory.VirtualAddress == 0x5000, "import section start changed");
		file_offset<NT>(dump, directory.VirtualAddress + directory.Size - 1);
		require(sections<NT>(dump)[0].Misc.VirtualSize == 0x5000, "fallback does not cover the import table");
	}

	template<typename NT>
	void file_only_metadata(bool win64)
	{
		auto image = mapped_fixture(win64, 0x2000);
		auto& header = nt<NT>(image);
		header.OptionalHeader.CheckSum = 0x1234;
		header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] = {0x2200, 0x200};
		header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] = {0x1100, sizeof(IMAGE_DEBUG_DIRECTORY)};
		auto& debug = *reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(image.data() + 0x1100);
		debug = {};
		debug.SizeOfData = 32;
		debug.AddressOfRawData = 0x1200;
		debug.PointerToRawData = 0x400;
		auto dump = reconstruct(image, false);
		const auto& output_header = nt<NT>(dump);
		const auto& certificate = output_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
		require(certificate.VirtualAddress == 0 && certificate.Size == 0, "unmapped certificate offset retained");
		require(output_header.OptionalHeader.CheckSum == 0, "stale checksum retained");
		const auto& output_debug = *reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(dump.data() + file_offset<NT>(dump, 0x1100));
		require(output_debug.PointerToRawData == file_offset<NT>(dump, 0x1200), "debug payload file offset was not relocated");
	}

	template<typename NT>
	void terminal_import(bool win64)
	{
		auto image = mapped_fixture(win64, 0x2000);
		const unsigned __int64 address = win64 ? 0x7fff12345678ULL : 0xf2345678ULL;
		const SIZE_T width = win64 ? 8 : 4;
		memcpy(image.data() + image.size() - width, &address, width);
		auto dump = reconstruct(image, true, address);
		const auto& directory = nt<NT>(dump).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		require(directory.Size == 2 * sizeof(IMAGE_IMPORT_DESCRIPTOR), "final pointer-sized import was omitted");
		const auto& descriptor = *reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(dump.data() + file_offset<NT>(dump, directory.VirtualAddress));
		require(descriptor.FirstThunk == image.size() - width, "terminal import references the wrong location");
	}

	template<typename NT>
	void layout_matrix(bool win64)
	{
		for (int variant = 0; variant < 16; ++variant)
		{
			auto image = mapped_fixture(win64, 0x40000);
			auto& header = nt<NT>(image);
			header.OptionalHeader.SizeOfHeaders = 0x400;
			header.FileHeader.NumberOfSections = 6;
			DWORD rva = 0x1000;
			for (int i = 0; i < 6; ++i)
			{
				auto& section = sections<NT>(image)[i];
				section = {};
				memcpy(section.Name, ".data", 5);
				section.VirtualAddress = rva;
				section.Misc.VirtualSize = 0x1000 + ((variant + i) % 4) * 0x1000 + i * 17;
				section.SizeOfRawData = i % 2 ? 0 : 0x200;
				section.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
				memset(image.data() + rva, 0, section.Misc.VirtualSize);
				if (i % 3)
				{
					image[rva + 17] = 0x5a;
					image[rva + section.Misc.VirtualSize - 1] = 0x6b;
				}
				rva = (rva + section.Misc.VirtualSize + 0xfff) & ~0xfff;
				rva += (variant % 3) * 0x1000;
			}
			header.OptionalHeader.SizeOfImage = rva;
			auto dump = reconstruct(image, variant % 2 != 0);
			require(nt<NT>(dump).OptionalHeader.SizeOfImage >= rva, "matrix virtual extent shrank");
			temporary_file source;
			source.write(dump);
			test_options options;
			pe_header reloaded(source.path, &options);
			require(reloaded.process_pe_header() && reloaded.process_sections(), "reconstructed layout cannot be reloaded");
			for (int i = 0; i < 6; ++i)
			{
				const auto& before = sections<NT>(image)[i];
				const auto& after = sections<NT>(dump)[i];
				require(after.VirtualAddress == before.VirtualAddress && after.Misc.VirtualSize == before.Misc.VirtualSize,
					"matrix section virtual layout changed");
				require(after.SizeOfRawData % 0x1000 == 0 && after.PointerToRawData % 0x1000 == 0,
					"matrix raw alignment invalid");
				require(range_fits(dump.size(), after.PointerToRawData, after.SizeOfRawData), "matrix raw range invalid");
				for (DWORD j = 0; j < before.Misc.VirtualSize; ++j)
				{
					const unsigned char value = j < after.SizeOfRawData ? dump[after.PointerToRawData + j] : 0;
					require(value == image[before.VirtualAddress + j], "matrix lost section content");
				}
			}
		}
	}
}

void append_reconstruction_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe32-sparse-data", [] { sparse_data<IMAGE_NT_HEADERS32>(false, false); }});
	tests.push_back({"pe64-sparse-data", [] { sparse_data<IMAGE_NT_HEADERS64>(true, false); }});
	tests.push_back({"pe32-sparse-data-imports", [] { sparse_data<IMAGE_NT_HEADERS32>(false, true); }});
	tests.push_back({"pe64-sparse-data-imports", [] { sparse_data<IMAGE_NT_HEADERS64>(true, true); }});
	tests.push_back({"pe32-sparse-rvas", [] { sparse_rvas<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-sparse-rvas", [] { sparse_rvas<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-unaligned-imports", [] { unaligned_imports<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-unaligned-imports", [] { unaligned_imports<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-large-section", [] { large_section<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-large-section", [] { large_section<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-full-header-imports", [] { full_header<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-full-header-imports", [] { full_header<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-file-metadata", [] { file_only_metadata<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-file-metadata", [] { file_only_metadata<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-terminal-import", [] { terminal_import<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-terminal-import", [] { terminal_import<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-layout-matrix", [] { layout_matrix<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-layout-matrix", [] { layout_matrix<IMAGE_NT_HEADERS64>(true); }});
}
