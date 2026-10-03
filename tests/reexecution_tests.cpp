#include "test_support.h"

namespace
{
	template<typename NT>
	NT& nt(std::vector<unsigned char>& bytes)
	{
		return *reinterpret_cast<NT*>(bytes.data() + 0x80);
	}

	template<typename NT>
	std::vector<unsigned char> fixture(bool win64)
	{
		auto bytes = pe_fixture(win64);
		bytes.resize(0x3200);
		auto& header = nt<NT>(bytes);
		header.FileHeader.NumberOfSections = 2;
		header.OptionalHeader.SizeOfImage = 0x4000;
		header.OptionalHeader.DllCharacteristics = IMAGE_DLLCHARACTERISTICS_GUARD_CF |
			IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
		auto& section = reinterpret_cast<IMAGE_SECTION_HEADER*>(&header + 1)[1];
		memcpy(section.Name, ".data", 5);
		section.VirtualAddress = 0x2000;
		section.Misc.VirtualSize = section.SizeOfRawData = 0x2000;
		section.PointerToRawData = 0x1200;
		section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		return bytes;
	}

	template<typename NT>
	SIZE_T file_offset(std::vector<unsigned char>& bytes, DWORD rva)
	{
		const auto& header = nt<NT>(bytes);
		const auto* sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(&header + 1);
		for (WORD i = 0; i < header.FileHeader.NumberOfSections; ++i)
			if (rva >= sections[i].VirtualAddress &&
				range_fits(sections[i].SizeOfRawData, rva - sections[i].VirtualAddress, 1))
				return sections[i].PointerToRawData + rva - sections[i].VirtualAddress;
		throw std::runtime_error("test RVA not present in reconstructed file");
	}

	std::vector<unsigned char> serialize(pe_header& image, bool expected_success = true)
	{
		temporary_file output;
		require(image.process_pe_header() && image.process_sections() && image.process_import_directory(),
			"reexecution fixture parsing failed");
		export_list exports;
		char library[] = "example.dll", name[] = "Example";
		export_entry entry(library, name, 1, 0x1000, 0x12345678, image.is_64());
		exports.add_export(0x12345678, &entry);
		require(image.process_disk_image(&exports, NULL) == expected_success, "unexpected preparation result");
		if (!expected_success)
			return {};
		require(DeleteFileA(output.path) && image.write_image(output.path), "prepared image write failed");
		return output.read();
	}

	std::vector<unsigned char> reconstruct(const std::vector<unsigned char>& bytes, bool prepare, bool imports,
		bool expected_success = true)
	{
		temporary_file input;
		input.write(bytes);
		test_options options;
		options.Reexecution = prepare;
		options.ImportRec = imports;
		pe_header image(input.path, &options);
		auto result = serialize(image, expected_success);
		require(input.read() == bytes, "preparation changed the source file");
		return result;
	}

	template<typename NT>
	std::vector<DWORD> import_slots(std::vector<unsigned char>& bytes);

	template<typename NT, typename CONFIG>
	void cookie_state(bool win64)
	{
		auto bytes = fixture<NT>(win64);
		const SIZE_T width = win64 ? 8 : 4;
		auto& config = *reinterpret_cast<CONFIG*>(bytes.data() + 0x1400);
		config.Size = sizeof(config);
		config.SecurityCookie = nt<NT>(bytes).OptionalHeader.ImageBase + 0x2a00;
		nt<NT>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = {0x2200, sizeof(config)};
		const ULONGLONG captured = win64 ? 0x123456789abcULL : 0x12345678ULL;
		memcpy(bytes.data() + 0x1c00, &captured, width);
		bytes[0x1bff] = bytes[0x1c00 + width] = 0x5a;
		for (bool imports : {false, true})
			for (bool prepare : {false, true})
			{
				auto dump = reconstruct(bytes, prepare, imports);
				const SIZE_T cookie = file_offset<NT>(dump, 0x2a00);
				ULONGLONG actual = 0;
				memcpy(&actual, dump.data() + cookie, width);
				const ULONGLONG expected = prepare ? (win64 ? 0x2b992ddfa232ULL : 0xbb40e64eULL) : captured;
				require(actual == expected && dump[cookie - 1] == 0x5a && dump[cookie + width] == 0x5a,
					"cookie reset was missing, unconditional, or exceeded the pointer width");
				require(nt<NT>(dump).OptionalHeader.DllCharacteristics == nt<NT>(bytes).OptionalHeader.DllCharacteristics,
					"preparation disabled a loader mitigation");
				if (prepare && imports)
				{
					const auto slots = import_slots<NT>(dump);
					require(std::find(slots.begin(), slots.end(), 0x2a00) == slots.end(),
						"GS cookie was mistaken for a function pointer");
				}
			}
	}

	template<typename NT, typename CONFIG>
	void cookie_bounds(bool win64)
	{
		auto bytes = fixture<NT>(win64);
		const SIZE_T width = win64 ? 8 : 4;
		auto& config = *reinterpret_cast<CONFIG*>(bytes.data() + 0x1400);
		config.Size = sizeof(config);
		nt<NT>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = {0x2200, sizeof(config)};
		const auto base = nt<NT>(bytes).OptionalHeader.ImageBase;
		for (DWORD rva : {0x100u, 0x1100u, 0x4000u, 0x3fffu})
		{
			config.SecurityCookie = base + rva;
			reconstruct(bytes, true, false, false);
		}
		config.SecurityCookie = base - 1;
		reconstruct(bytes, true, false, false);
		config.SecurityCookie = 0;
		reconstruct(bytes, true, false);
		config.SecurityCookie = base + 0x2a00;
		config.Size = offsetof(CONFIG, SecurityCookie);
		bytes[0x1c00] = 0x5a;
		auto old_version = reconstruct(bytes, true, false);
		require(old_version[file_offset<NT>(old_version, 0x2a00)] == 0x5a, "older load-config version was overread");
		config.Size = sizeof(config);
		config.SecurityCookie = base + 0x4000 - width;
		bytes[bytes.size() - width] = 0x5a;
		auto terminal = reconstruct(bytes, true, false);
		ULONGLONG actual = 0;
		memcpy(&actual, terminal.data() + file_offset<NT>(terminal, static_cast<DWORD>(0x4000 - width)), width);
		require(actual == (win64 ? 0x2b992ddfa232ULL : 0xbb40e64eULL), "terminal cookie was rejected or truncated");
		nt<NT>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = 0x3ff0;
		reconstruct(bytes, true, false, false);
	}

	template<typename NT>
	std::vector<DWORD> import_slots(std::vector<unsigned char>& bytes)
	{
		const auto& directory = nt<NT>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		const SIZE_T offset = file_offset<NT>(bytes, directory.VirtualAddress);
		std::vector<DWORD> result;
		for (SIZE_T i = 0; i < directory.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR); ++i)
		{
			IMAGE_IMPORT_DESCRIPTOR item;
			memcpy(&item, bytes.data() + offset + i * sizeof(item), sizeof(item));
			if (item.FirstThunk == 0)
				return result;
			result.push_back(item.FirstThunk);
		}
		throw std::runtime_error("prepared imports lack a terminator");
	}

	template<typename NT>
	void put_fixups(std::vector<unsigned char>& bytes, bool win64)
	{
		const SIZE_T width = win64 ? 8 : 4;
		const ULONGLONG address = 0x12345678;
		const DWORD slots[] = {0x1100, 0x2400, static_cast<DWORD>(0x2400 + width), static_cast<DWORD>(0x4000 - width)};
		for (DWORD rva : slots)
			memcpy(bytes.data() + file_offset<NT>(bytes, rva), &address, width);
	}

	template<typename NT>
	void conservative_imports(bool win64)
	{
		auto bytes = fixture<NT>(win64);
		put_fixups<NT>(bytes, win64);
		const DWORD width = win64 ? 8 : 4;
		auto normal = reconstruct(bytes, false, true);
		auto prepared = reconstruct(bytes, true, true);
		require(import_slots<NT>(normal) == std::vector<DWORD>({0x1100, 0x2400, 0x2400 + width, 0x4000 - width}),
			"normal aggressive reconstruction changed");
		require(import_slots<NT>(prepared) == std::vector<DWORD>({0x2400, 0x2400 + width, 0x4000 - width}),
			"conservative imports dropped independent writable references or included code");
		require(memcmp(prepared.data() + file_offset<NT>(prepared, 0x1100), bytes.data() + 0x300, width) == 0,
			"conservative scanning rewrote code bytes");
		auto& data = reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt<NT>(bytes) + 1)[1];
		for (DWORD flags : {static_cast<DWORD>(IMAGE_SCN_MEM_READ),
			static_cast<DWORD>(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE)})
		{
			data.Characteristics = flags;
			auto excluded = reconstruct(bytes, true, true);
			require(import_slots<NT>(excluded).empty(), "readonly or executable import destination included");
		}
	}

	template<typename NT>
	void delay_imports(bool win64, bool malformed)
	{
		auto bytes = fixture<NT>(win64);
		put_fixups<NT>(bytes, win64);
		auto& directory = nt<NT>(bytes).OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
		directory = {0x2800, 64};
		auto* descriptor = reinterpret_cast<DWORD*>(bytes.data() + 0x1a00);
		descriptor[0] = 1;
		descriptor[3] = 0x2400;
		if (malformed)
		{
			for (DWORD iat : {0u, 0x4000u, 0xffffffffu})
			{
				descriptor[3] = iat;
				reconstruct(bytes, true, true, false);
			}
			descriptor[3] = 0x2400;
			directory.Size = 32;
			reconstruct(bytes, true, true, false);
			directory = {0x3ff0, 64};
			reconstruct(bytes, true, true, false);
			return;
		}
		auto prepared = reconstruct(bytes, true, true);
		require(import_slots<NT>(prepared).empty(), "protected delay-IAT section became a normal import destination");
		auto normal = reconstruct(bytes, false, true);
		require(import_slots<NT>(normal).size() == 4, "delay imports changed the default analysis mode");
		if (!win64)
		{
			descriptor[0] = 0;
			descriptor[3] = static_cast<DWORD>(nt<NT>(bytes).OptionalHeader.ImageBase) + 0x2400;
			auto legacy = reconstruct(bytes, true, true);
			require(import_slots<NT>(legacy).empty(), "legacy VA delay IAT was not excluded");
		}
	}

	template<typename NT, typename CONFIG>
	void zero_fill(bool win64)
	{
		for (DWORD flags : {static_cast<DWORD>(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE),
			static_cast<DWORD>(IMAGE_SCN_MEM_READ), static_cast<DWORD>(IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE)})
			for (DWORD raw_size : {0u, 0x1000u})
				for (bool imports : {false, true})
					for (bool occupied : {false, true})
						for (bool prepare : {false, true})
						{
							auto file = fixture<NT>(win64);
							std::vector<unsigned char> memory(0x4000);
							memcpy(memory.data(), file.data(), 0x200);
							memcpy(memory.data() + 0x1000, file.data() + 0x200, 0x1000);
							auto& header = nt<NT>(memory);
							auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(&header + 1);
							auto& data = sections[1];
							data.SizeOfRawData = raw_size;
							data.Characteristics = flags | (raw_size ? IMAGE_SCN_CNT_INITIALIZED_DATA : IMAGE_SCN_CNT_UNINITIALIZED_DATA);
							if (occupied)
								reinterpret_cast<unsigned char*>(sections + 2)[0] = 0x5a;
							const bool writable = (flags & IMAGE_SCN_MEM_WRITE) && !(flags & IMAGE_SCN_MEM_EXECUTE);
							const SIZE_T width = win64 ? 8 : 4;
							const ULONGLONG address = 0x12345678, captured_cookie = 0x1122334455667788ULL;
							for (DWORD rva : {0x2400u, 0x3200u})
								memcpy(memory.data() + rva, &address, width);
							memory[0x2800] = 0x5a;
							if (writable)
							{
								auto& config = *reinterpret_cast<CONFIG*>(memory.data() + 0x1400);
								memset(&config, 0, sizeof(config));
								config.Size = sizeof(config);
								config.SecurityCookie = static_cast<decltype(config.SecurityCookie)>(
									reinterpret_cast<uintptr_t>(memory.data()) + 0x4000 - width);
								header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = {0x1400, sizeof(config)};
								memcpy(memory.data() + 0x4000 - width, &captured_cookie, width);
							}
							const auto before = memory;
							test_options options;
							options.Reexecution = prepare;
							options.ImportRec = imports;
							pe_header image(GetCurrentProcess(), memory.data(), NULL, &options);
							auto dump = serialize(image);
							require(memory == before, "zero-fill restoration changed captured memory");
							const auto& output_data = reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt<NT>(dump) + 1)[1];
							const auto byte_at = [&](DWORD rva) {
								return rva - output_data.VirtualAddress < output_data.SizeOfRawData ?
									dump[file_offset<NT>(dump, rva)] : static_cast<unsigned char>(0);
							};
							const bool reset = prepare && writable;
							require(byte_at(0x3200) == (reset ? 0 : 0x78), "zero-fill data was not restored only when requested");
							require(byte_at(0x2800) == (reset && raw_size == 0 ? 0 : 0x5a),
								"file-backed data or a fully zero-fill section was handled incorrectly");
							if (imports)
							{
								const std::vector<DWORD> expected = !prepare ? std::vector<DWORD>({0x2400, 0x3200}) :
									(writable && raw_size != 0 ? std::vector<DWORD>({0x2400}) : std::vector<DWORD>());
								require(import_slots<NT>(dump) == expected, "zero-fill data became a speculative import");
							}
							if (writable)
							{
								ULONGLONG actual = 0;
								memcpy(&actual, dump.data() + file_offset<NT>(dump, static_cast<DWORD>(0x4000 - width)), width);
								const ULONGLONG expected = prepare ? (win64 ? 0x2b992ddfa232ULL : 0xbb40e64eULL) :
									(win64 ? captured_cookie : static_cast<DWORD>(captured_cookie));
								require(actual == expected, "a cookie in zero-fill storage was lost or not initialized");
							}
						}
		auto file = fixture<NT>(win64);
		std::vector<unsigned char> memory(0x4000);
		memcpy(memory.data(), file.data(), 0x200);
		auto& header = nt<NT>(memory);
		header.FileHeader.NumberOfSections = 3;
		auto* sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(&header + 1);
		sections[1].SizeOfRawData = 0x1000;
		sections[1].Misc.VirtualSize = 0x3000;
		sections[2] = {};
		sections[2].VirtualAddress = 0x3000;
		sections[2].Misc.VirtualSize = 0x1000;
		test_options options;
		options.Reexecution = true;
		pe_header image(GetCurrentProcess(), memory.data(), NULL, &options);
		serialize(image, false);
	}
}

void append_reexecution_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe32-reexecution-cookie", [] { cookie_state<IMAGE_NT_HEADERS32, IMAGE_LOAD_CONFIG_DIRECTORY32>(false); }});
	tests.push_back({"pe64-reexecution-cookie", [] { cookie_state<IMAGE_NT_HEADERS64, IMAGE_LOAD_CONFIG_DIRECTORY64>(true); }});
	tests.push_back({"pe32-reexecution-cookie-bounds", [] { cookie_bounds<IMAGE_NT_HEADERS32, IMAGE_LOAD_CONFIG_DIRECTORY32>(false); }});
	tests.push_back({"pe64-reexecution-cookie-bounds", [] { cookie_bounds<IMAGE_NT_HEADERS64, IMAGE_LOAD_CONFIG_DIRECTORY64>(true); }});
	tests.push_back({"pe32-reexecution-imports", [] { conservative_imports<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-reexecution-imports", [] { conservative_imports<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-reexecution-delay-imports", [] { delay_imports<IMAGE_NT_HEADERS32>(false, false); }});
	tests.push_back({"pe64-reexecution-delay-imports", [] { delay_imports<IMAGE_NT_HEADERS64>(true, false); }});
	tests.push_back({"pe32-reexecution-delay-bounds", [] { delay_imports<IMAGE_NT_HEADERS32>(false, true); }});
	tests.push_back({"pe64-reexecution-delay-bounds", [] { delay_imports<IMAGE_NT_HEADERS64>(true, true); }});
	tests.push_back({"pe32-reexecution-zero-fill", [] { zero_fill<IMAGE_NT_HEADERS32, IMAGE_LOAD_CONFIG_DIRECTORY32>(false); }});
	tests.push_back({"pe64-reexecution-zero-fill", [] { zero_fill<IMAGE_NT_HEADERS64, IMAGE_LOAD_CONFIG_DIRECTORY64>(true); }});
}
