#pragma once

#include "stdafx.h"
#include <stdio.h>
#include "windows.h"
#include "stream_wrapper.h"
#include <stdlib.h>
#include "module_list.h"
#include "export_list.h"
#include <unordered_map>
#include <unordered_set>
#include "string.h"
#include "pe_imports.h"
#include <functional>

using namespace std;
using namespace std::tr1;


#define FILEPATH_SIZE 265

// 10MB
#define MAX_SECTION_SIZE (1024 * 1000) * 60

static bool static_zero_init = false;
static char static_zero [100];

enum ALIGNMENT
{
	PHYSICAL,
	VIRTUAL
};

struct IMPORT_SUMMARY
{
	size_t COUNT_UNIQUE_IMPORT_ADDRESSES;
	size_t COUNT_UNIQUE_IMPORT_LIBRARIES;
	unsigned __int64 HASH_GENERIC;
	unsigned __int64 HASH_SPECIFIC;
};

class pe_header
{
	
	unsigned __int64 _unique_hash;

	PD_OPTIONS* _options;

	stream_wrapper* _stream;
	void* _original_base;


	SIZE_T _raw_header_size;
	unsigned char* _raw_header;
	SIZE_T _image_size;
	unsigned char* _image;
	SIZE_T _disk_image_size;
	unsigned char* _disk_image;
	

	// Extracted current and original filename information about this module
	SIZE_T _name_filepath_short_size;
	char* _name_filepath_short;
	SIZE_T _name_filepath_long_size;
	char* _name_filepath_long;
	SIZE_T _name_original_exports_size;
	char* _name_original_exports;
	SIZE_T _name_original_manifest_size;
	char* _name_original_manifest;
	SIZE_T _name_symbols_path_size;
	char* _name_symbols_path;


	bool _parsed_dos;
	IMAGE_DOS_HEADER* _header_dos;

	bool _parsed_pe_32;
	IMAGE_NT_HEADERS32* _header_pe32;
	bool _parsed_pe_64;
	IMAGE_NT_HEADERS64* _header_pe64;
	int _correction_offset;

	// Import table
	int _header_import_descriptors_count;
	IMAGE_IMPORT_DESCRIPTOR* _header_import_descriptors;

	// Export table
	IMAGE_EXPORT_DIRECTORY* _header_export_directory;
	export_list* _export_list;
	
	bool _parsed_sections;
	int _num_sections;
	IMAGE_SECTION_HEADER* _header_sections;
	DWORD* _header_section_sizes;

	bool _test_read( unsigned char* buffer, SIZE_T length, unsigned char* read_ptr, SIZE_T read_length );
	
	
	
	
	DWORD _section_align( DWORD address, DWORD alignment);
	__int64 _section_align( __int64 address, DWORD alignment);

public:
	pe_header( char* filename, PD_OPTIONS* options );
	pe_header( DWORD pid, void* base, module_list* modules, PD_OPTIONS* options );
	pe_header( DWORD pid, module_list* modules, PD_OPTIONS* options );
	pe_header( HANDLE ph, void* base, module_list* modules, PD_OPTIONS* options );
	
	bool build_pe_header( __int64 size, bool amd64 );
	bool build_pe_header( __int64 size, bool amd64, int num_sections_limit );
	
	bool process_pe_header( );
	bool process_import_directory( );
	bool process_export_directory( );
	bool process_relocation_directory();
	bool process_sections( );
	bool process_disk_image( export_list* exports );
	bool process_hash( );

	bool somewhat_parsed();

	unsigned __int64 get_hash();

	IMPORT_SUMMARY get_imports_information( export_list* exports );
	IMPORT_SUMMARY get_imports_information( export_list* exports, __int64 size_limit );

	bool write_image( char* filename );

	export_list* get_exports();
	unsigned __int64 get_virtual_size();

	bool is_64();
	bool is_dll();
	bool is_exe();
	bool is_sys();
	char* get_name();
	void set_name(char* new_name);

	void print_report( FILE* stream );

	__int64 get_export_addresses();
	
	~pe_header(void);
};
