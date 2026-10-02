#pragma once
#include "windows.h"
#include "utils.h"
#include <memory>
#include <vector>

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL
#define IMAGE_ORDINAL_FLAG32 0x80000000

class import_library
{
	char* _library_name = NULL;
	
	IMAGE_IMPORT_DESCRIPTOR* _descriptor = NULL;
	IMAGE_THUNK_DATA64* _thunk_entry = NULL; // Use 64 bit definition for both 32 and 64 bit modules.
	IMAGE_IMPORT_BY_NAME* _import_by_name = NULL;
	SIZE_T _import_by_name_len = 0;
	SIZE_T _library_name_len = 0;

public:
	import_library(IMAGE_IMPORT_DESCRIPTOR* descriptor, bool win64);
	import_library(char* library_name, int ordinal, __int64 rva, bool win64);
	import_library(char* library_name, char* proc_name, __int64 rva, bool win64);

	bool build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 &descriptor_offset, __int64 &extra_offset);
	void get_table_size(__int64 &descriptor_size, __int64 &extra_size);
	bool valid() const { return _descriptor != NULL; }
	char* GetName();
	~import_library(void);
	import_library(const import_library&) = delete;
	import_library& operator=(const import_library&) = delete;
};

class pe_imports
{
	bool _win64;
	bool _valid = true;
	SIZE_T _owned_size = sizeof(IMAGE_IMPORT_DESCRIPTOR);
	std::vector<std::unique_ptr<import_library>> _libraries;
	void _add(std::unique_ptr<import_library> library);
public:
	pe_imports(unsigned char* image, __int64 image_size, IMAGE_IMPORT_DESCRIPTOR* imports, bool win64);
	void add_descriptor(IMAGE_IMPORT_DESCRIPTOR* descriptor);
	bool build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 descriptor_offset, __int64 extra_offset);
	void get_table_size(__int64 &descriptor_size, __int64 &extra_size);
	bool valid() const { return _valid; }

	void add_fixup(char* library_name, int ordinal, __int64 rva, bool win64);
	void add_fixup(char* library_name, char* proc_name, __int64 rva, bool win64);
	//char* build_table();
	~pe_imports(void);
};

