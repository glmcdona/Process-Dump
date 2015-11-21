#pragma once

#include "DynArray.h"
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <unordered_map>
#include "utils.h"

using namespace std;
using namespace std::tr1;

class export_entry
{
public:
	char* library_name;
	char* name;
	WORD ord;
	__int64 rva;
	__int64 address;

	export_entry(char* library_name, char* name, WORD ord, __int64 rva, __int64 address);
	export_entry(export_entry* other);
	~export_entry(void);
};


class export_list
{

public:
	unordered_map<__int64,export_entry*> address_to_exports; // List of export addresses in this export list

	export_list();
	
	bool add_exports(unsigned char* image, SIZE_T image_size, __int64 image_base, IMAGE_EXPORT_DIRECTORY* header_export_directory);
	bool add_exports(export_list* other);

	~export_list(void);
};
