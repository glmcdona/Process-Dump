#include "test_support.h"
#include <limits.h>

namespace
{
	class fixture_file
	{
	public:
		char path[MAX_PATH];
		fixture_file()
		{
			static LONG sequence = 0;
			sprintf_s(path, "pe-safety-%lu-%ld.fixture", GetCurrentProcessId(), InterlockedIncrement(&sequence));
		}
		~fixture_file() { DeleteFileA(path); }
		void write(const std::vector<unsigned char>& bytes)
		{
			FILE* file = fopen(path, "wb");
			require(file != NULL, "fixture open failed");
			const bool written = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
			const int closed = fclose(file);
			require(written && closed == 0, "fixture write failed");
		}
	};

	template<typename NT>
	NT& nt_header(std::vector<unsigned char>& bytes)
	{
		return *reinterpret_cast<NT*>(bytes.data() + 0x80);
	}

	template<typename NT>
	IMAGE_SECTION_HEADER& first_section(std::vector<unsigned char>& bytes)
	{
		return *reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt_header<NT>(bytes) + 1);
	}

	template<typename NT>
	void reject_sections(bool win64, const std::function<void(std::vector<unsigned char>&)>& modify)
	{
		std::vector<unsigned char> bytes = pe_fixture(win64);
		modify(bytes);
		fixture_file source;
		source.write(bytes);
		test_options options;
		pe_header header(source.path, &options);
		require(header.process_pe_header(), "ordinary fixture header rejected");
		require(!header.process_sections(), "invalid section metadata accepted");
		require(header.get_virtual_size() == 0, "failed section parse retained an image");
		export_list exports;
		require(!header.process_disk_image(&exports, NULL), "failed section parse permitted reconstruction");
	}

	template<typename NT>
	void section_rejections(bool win64)
	{
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			nt_header<NT>(bytes).FileHeader.NumberOfSections = 0;
		});
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			first_section<NT>(bytes).SizeOfRawData = 0x2000;
		});
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			nt_header<NT>(bytes).OptionalHeader.SizeOfImage = static_cast<DWORD>(MAX_PE_IMAGE_SIZE + 1);
		});
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			first_section<NT>(bytes).VirtualAddress = static_cast<DWORD>(MAX_PE_IMAGE_SIZE - 0x1000);
			first_section<NT>(bytes).Misc.VirtualSize = 0x2000;
		});
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			first_section<NT>(bytes).PointerToRawData = static_cast<DWORD>(LONG_MAX) + 1;
		});
		reject_sections<NT>(win64, [](std::vector<unsigned char>& bytes) {
			nt_header<NT>(bytes).FileHeader.NumberOfSections = 2;
			IMAGE_SECTION_HEADER* sections = &first_section<NT>(bytes);
			sections[1] = sections[0];
			sections[0].VirtualAddress = 0x3000;
		});
	}

	template<typename NT>
	void alignment_rejections(bool win64)
	{
		for (int case_index = 0; case_index < 4; ++case_index)
		{
			std::vector<unsigned char> bytes = pe_fixture(win64);
			NT& nt = nt_header<NT>(bytes);
			if (case_index == 0)
				nt.OptionalHeader.SectionAlignment = 0;
			else if (case_index == 1)
				nt.OptionalHeader.FileAlignment = 0;
			else
				nt.OptionalHeader.SectionAlignment = static_cast<DWORD>(MAX_PE_IMAGE_SIZE);
			fixture_file source;
			source.write(bytes);
			test_options options;
			options.ImportRec = case_index != 3;
			pe_header header(source.path, &options);
			require(header.process_pe_header() && header.process_sections(), "ordinary sections rejected");
			export_list exports;
			require(!header.process_disk_image(&exports, NULL), "invalid alignment or oversized reconstruction accepted");
		}
	}

	void dos_offset_rejections()
	{
		for (LONG offset : {-1L, 0x1200L})
		{
			std::vector<unsigned char> bytes = pe_fixture(false);
			reinterpret_cast<IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew = offset;
			fixture_file source;
			source.write(bytes);
			test_options options;
			pe_header header(source.path, &options);
			require(!header.process_pe_header(), "out-of-range DOS header offset accepted");
			require(!header.process_sections(), "invalid header permitted section parsing");
		}
	}

	void range_boundaries()
	{
		unsigned char bytes[16] = {};
		require(range_fits(sizeof(bytes), sizeof(bytes), 0), "empty end range rejected");
		require(range_fits(sizeof(bytes), 8, 8), "exact range rejected");
		require(!range_fits(sizeof(bytes), 8, 9), "oversized range accepted");
		require(!range_fits(sizeof(bytes), 17, 0), "offset beyond end accepted");
		require(range_fits(SIZE_MAX, SIZE_MAX, 0), "maximum empty range rejected");
		require(!range_fits(SIZE_MAX, SIZE_MAX, 1), "maximum nonempty range accepted");
		require(test_read(bytes, sizeof(bytes), bytes + 8, 8), "exact pointer range rejected");
		require(!test_read(bytes, sizeof(bytes), bytes + 8, 9), "pointer range beyond end accepted");
		require(!test_read(NULL, 0, NULL, 0), "null buffer accepted");
	}

	void import_builder_boundaries()
	{
		char library[] = "kernel32.dll", function[] = "ExitProcess";
		for (bool win64 : {false, true})
		{
			pe_imports imports(NULL, 0, NULL, win64);
			imports.add_fixup(library, function, 0x1000, win64);
			__int64 descriptors = 0, extra = 0;
			imports.get_table_size(descriptors, extra);
			std::vector<unsigned char> bytes(static_cast<SIZE_T>(descriptors + extra), 0xcc);
			const std::vector<unsigned char> unchanged = bytes;
			require(!imports.build_table(bytes.data(), bytes.size() - 1, 0x3000, 0, descriptors), "short table accepted");
			require(bytes == unchanged, "rejected table modified destination");
			require(!imports.build_table(bytes.data(), bytes.size(), 0x3000, -1, descriptors), "negative descriptor offset accepted");
			require(!imports.build_table(bytes.data(), bytes.size(), 0x3000, 0, -1), "negative extra offset accepted");
			require(!imports.build_table(bytes.data(), bytes.size(), 0x3000, 0, descriptors - 1), "overlapping table regions accepted");
			require(!imports.build_table(bytes.data(), bytes.size(), MAXDWORD, 0, descriptors), "unrepresentable table RVA accepted");
			require(!imports.build_table(bytes.data(), -1, 0x3000, 0, descriptors), "negative section size accepted");
			require(bytes == unchanged, "invalid table request wrote bytes");
			require(imports.build_table(bytes.data(), bytes.size(), 0x3000, 0, descriptors), "exact table size rejected");
			const std::vector<unsigned char> built = bytes;
			require(imports.build_table(bytes.data(), bytes.size(), 0x3000, 0, descriptors), "second table build failed");
			require(bytes == built, "repeated table build changed bytes");
		}
		pe_imports empty(NULL, 0, NULL, false);
		unsigned char terminator[sizeof(IMAGE_IMPORT_DESCRIPTOR)] = {};
		require(!empty.build_table(terminator, sizeof(terminator) - 1, 0, 0, sizeof(terminator)), "missing terminator space accepted");
		require(empty.build_table(terminator, sizeof(terminator), 0, 0, sizeof(terminator)), "exact terminator space rejected");
		pe_imports invalid(NULL, -1, NULL, false);
		require(!invalid.valid(), "negative image length accepted");
	}

	void repeated_parsing()
	{
		for (bool win64 : {false, true})
		{
			fixture_file source;
			source.write(pe_fixture(win64));
			test_options options;
			options.ImportRec = true;
			pe_header header(source.path, &options);
			export_list exports;
			for (int iteration = 0; iteration < 3; ++iteration)
			{
				require(header.process_pe_header(), "repeated header parse failed");
				require(header.process_sections() && header.process_sections(), "repeated section parse failed");
				require(header.process_import_directory(), "repeated import parse failed");
				header.process_export_directory();
				header.process_export_directory();
				require(header.process_disk_image(&exports, NULL), "repeated reconstruction failed");
			}
			require(!header.build_pe_header(static_cast<__int64>(MAX_PE_IMAGE_SIZE) + 1, win64), "oversized generated image accepted");
			require(!header.somewhat_parsed() && header.get_virtual_size() == 0, "failed reparse retained stale state");
		}
	}
}

void append_pe_safety_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe-range-boundaries", range_boundaries});
	tests.push_back({"pe-dos-offset-rejection", dos_offset_rejections});
	tests.push_back({"pe32-section-rejections", [] { section_rejections<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-section-rejections", [] { section_rejections<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe32-alignment-rejections", [] { alignment_rejections<IMAGE_NT_HEADERS32>(false); }});
	tests.push_back({"pe64-alignment-rejections", [] { alignment_rejections<IMAGE_NT_HEADERS64>(true); }});
	tests.push_back({"pe-import-builder-boundaries", import_builder_boundaries});
	tests.push_back({"pe-repeated-parsing", repeated_parsing});
}
