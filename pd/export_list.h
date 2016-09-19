#pragma once

#include "DynArray.h"
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "utils.h"
#include "limits.h"

using namespace std;
using namespace std::tr1;

class export_entry
{
public:
	char* library_name;
	char* name;
	WORD ord;
	bool is64;
	unsigned __int64 rva;
	unsigned __int64 address;

	export_entry(char* library_name, char* name, WORD ord, unsigned __int64 rva, unsigned __int64 address, bool is64);
	export_entry(export_entry* other);
	~export_entry(void);
};


class export_list
{
	unsigned __int64 _min64;
	unsigned __int64 _max64;
	unsigned __int32 _min32;
	unsigned __int32 _max32;
	unsigned __int32 _bits32;
	unsigned __int64 _bits64;

	unordered_map<unsigned __int64, export_entry*> _address_to_exports; // List of export addresses in this export list
	unordered_set<unsigned __int64> _addresses; // List of export addresses
public:
	

	export_list();
	
	bool add_exports(unsigned char* image, SIZE_T image_size, unsigned __int64 image_base, IMAGE_EXPORT_DIRECTORY* header_export_directory, bool is64);
	bool add_exports(export_list* other);
	void add_export(unsigned __int64 address, export_entry* entry);

	// Find export addresses in a process
	unsigned __int64 find_export(char* library, char* name, bool is64);

	// Functions to get quick filter values before doing a lookup
	bool contains(unsigned __int64 address);
	bool contains(unsigned __int32 address);
	export_entry find(unsigned __int64 address);
	unsigned __int64 get_min64() { return _min64; };
	unsigned __int64 get_max64() { return _max64; };
	unsigned __int32 get_min32() { return _min32; };
	unsigned __int32 get_max32() { return _max32; };
	unsigned __int32 get_nobits32() { return ~_bits32; };
	unsigned __int64 get_nobits64() { return ~_bits64; };

	~export_list(void);
};
