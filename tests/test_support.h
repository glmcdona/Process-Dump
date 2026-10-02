#pragma once

#include "pe_header.h"
#include <vector>
#include <stdexcept>
#include <string>
#include <functional>

using test_case = std::pair<const char*, std::function<void()>>;

void append_stream_tests(std::vector<test_case>& tests);
void append_pe_safety_tests(std::vector<test_case>& tests);
void append_name_safety_tests(std::vector<test_case>& tests);
void append_hook_tests(std::vector<test_case>& tests);
void append_output_tests(std::vector<test_case>& tests);
void append_reconstruction_tests(std::vector<test_case>& tests);
void append_pipeline_tests(std::vector<test_case>& tests);
void append_performance_tests(std::vector<test_case>& tests);
void append_reexecution_tests(std::vector<test_case>& tests);
void append_scheduling_tests(std::vector<test_case>& tests);
void append_entrypoint_tests(std::vector<test_case>& tests);
int run_performance_benchmark(int argc, char** argv);
int run_reexecution_probe(int argc, char** argv);
int run_reexecution_fixture();
int run_entrypoint_benchmark(int argc, char** argv);
int run_system_fixture();
int run_system_benchmark(int argc, char** argv);

inline void require(bool condition, const char* message)
{
	if (!condition)
		throw std::runtime_error(message);
}

class test_options : public PD_OPTIONS
{
public:
	test_options()
	{
		ImportRec = false;
		ForceGenHeader = false;
		Verbose = false;
		ReconstructHeaderAsDll = false;
		DumpChunks = false;
		EntryPointHash = false;
		ForceReconstructEntryPoint = false;
		NumberOfThreads = 1;
		EntryPointOverride = 0;
	}
};

class temporary_file
{
public:
	char path[MAX_PATH];
	temporary_file()
	{
		char folder[MAX_PATH];
		require(GetTempPathA(MAX_PATH, folder) != 0, "GetTempPath failed");
		require(GetTempFileNameA(folder, "pdt", 0, path) != 0, "GetTempFileName failed");
	}
	~temporary_file() { DeleteFileA(path); }
	void write(const std::vector<unsigned char>& bytes)
	{
		FILE* file = fopen(path, "wb");
		require(file != NULL, "fixture open failed");
		const bool written = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
		const int closed = fclose(file);
		require(written && closed == 0, "fixture write failed");
	}
	std::vector<unsigned char> read()
	{
		FILE* file = fopen(path, "rb");
		require(file != NULL, "fixture read open failed");
		fseek(file, 0, SEEK_END);
		const long size = ftell(file);
		rewind(file);
		std::vector<unsigned char> bytes(size);
		const bool read = fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
		fclose(file);
		require(read, "fixture read failed");
		return bytes;
	}
	temporary_file(const temporary_file&) = delete;
	temporary_file& operator=(const temporary_file&) = delete;
};

inline std::vector<unsigned char> pe_fixture(bool win64)
{
	std::vector<unsigned char> bytes(0x1200, 0);
	IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(bytes.data());
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = 0x80;
	IMAGE_SECTION_HEADER* section;
	if (win64)
	{
		IMAGE_NT_HEADERS64* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(bytes.data() + 0x80);
		nt->Signature = IMAGE_NT_SIGNATURE;
		nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
		nt->FileHeader.NumberOfSections = 1;
		nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
		nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
		nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
		nt->OptionalHeader.ImageBase = 0x140000000;
		nt->OptionalHeader.AddressOfEntryPoint = 0x1000;
		nt->OptionalHeader.SectionAlignment = 0x1000;
		nt->OptionalHeader.FileAlignment = 0x200;
		nt->OptionalHeader.SizeOfHeaders = 0x200;
		nt->OptionalHeader.SizeOfImage = 0x2000;
		nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		section = reinterpret_cast<IMAGE_SECTION_HEADER*>(nt + 1);
	}
	else
	{
		IMAGE_NT_HEADERS32* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(bytes.data() + 0x80);
		nt->Signature = IMAGE_NT_SIGNATURE;
		nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
		nt->FileHeader.NumberOfSections = 1;
		nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
		nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
		nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
		nt->OptionalHeader.ImageBase = 0x400000;
		nt->OptionalHeader.AddressOfEntryPoint = 0x1000;
		nt->OptionalHeader.SectionAlignment = 0x1000;
		nt->OptionalHeader.FileAlignment = 0x200;
		nt->OptionalHeader.SizeOfHeaders = 0x200;
		nt->OptionalHeader.SizeOfImage = 0x2000;
		nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		section = reinterpret_cast<IMAGE_SECTION_HEADER*>(nt + 1);
	}
	memcpy(section->Name, ".text", 5);
	section->VirtualAddress = 0x1000;
	section->Misc.VirtualSize = 0x1000;
	section->PointerToRawData = 0x200;
	section->SizeOfRawData = 0x1000;
	section->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
	memset(bytes.data() + 0x200, 0x90, 0x1000);
	bytes[0x11ff] = 0xc3;
	return bytes;
}
