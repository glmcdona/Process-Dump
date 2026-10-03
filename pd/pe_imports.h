#pragma once
#include "windows.h"
#include "utils.h"
#include "export_list.h"
#include <memory>
#include <vector>

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL
#define IMAGE_ORDINAL_FLAG32 0x80000000

class import_library
{
	IMAGE_THUNK_DATA64 _thunk_entry = {};
	IMAGE_IMPORT_DESCRIPTOR _descriptor = {};
	DWORD _library_name_len = 0;
	DWORD _proc_name_len = 0;
	bool _valid = false;
	const char* _library_name = NULL;
	const char* _proc_name = NULL;
	std::unique_ptr<char[]> _owned_names;
	void _initialize(const char* library_name, const char* proc_name, int ordinal, __int64 rva, bool win64);
	void _copy_names();

public:
	import_library(IMAGE_IMPORT_DESCRIPTOR* descriptor, bool win64);
	import_library(char* library_name, int ordinal, __int64 rva, bool win64);
	import_library(char* library_name, char* proc_name, __int64 rva, bool win64);
	// Borrowed names remain valid while the immutable export list is alive.
	import_library(const export_entry& entry, __int64 rva, bool win64);

	bool build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 &descriptor_offset, __int64 &extra_offset);
	void get_table_size(__int64 &descriptor_size, __int64 &extra_size);
	bool valid() const { return _valid; }
	import_library(import_library&&) noexcept = default;
	import_library& operator=(import_library&&) noexcept = default;
	import_library(const import_library&) = delete;
	import_library& operator=(const import_library&) = delete;
};

class pe_imports
{
	bool _win64;
	bool _valid = true;
	SIZE_T _owned_size = sizeof(IMAGE_IMPORT_DESCRIPTOR);
	std::vector<import_library> _libraries;
	void _add(import_library library);
public:
	pe_imports(unsigned char* image, __int64 image_size, IMAGE_IMPORT_DESCRIPTOR* imports, bool win64);
	void add_descriptor(IMAGE_IMPORT_DESCRIPTOR* descriptor);
	bool build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 descriptor_offset, __int64 extra_offset);
	void get_table_size(__int64 &descriptor_size, __int64 &extra_size);
	bool valid() const { return _valid; }

	void add_fixup(char* library_name, int ordinal, __int64 rva, bool win64);
	void add_fixup(char* library_name, char* proc_name, __int64 rva, bool win64);
	void add_fixup(const export_entry& entry, __int64 rva, bool win64);
	//char* build_table();
	~pe_imports(void);
};
