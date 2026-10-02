#include "StdAfx.h"
#include "export_list.h"
#include <string>


export_entry::export_entry(const char* library_name, const char* name, WORD ord, unsigned __int64 rva, unsigned __int64 address, bool is64)
{
	// Copy the strings locally, knowing the function might not have a name but has to have a library name
	if (library_name != NULL)
	{
		this->library_name = new char[strlen(library_name) + 1];
		strcpy(this->library_name, library_name);
	}
	else
	{
		this->library_name = NULL;
	}

	if( name != NULL )
	{
		this->name = new char[strlen(name) + 1];
		strcpy( this->name, name );
	}
	else
	{
		this->name = NULL;
	}

	this->is64 = is64;
	this->ord = ord;
	this->rva = rva;
	this->address = address;
}

export_entry::export_entry(export_entry* other)
	: export_entry(other == NULL ? NULL : other->library_name,
		other == NULL ? NULL : other->name,
		other == NULL ? 0 : other->ord,
		other == NULL ? 0 : other->rva,
		other == NULL ? 0 : other->address,
		other != NULL && other->is64)
{
}

export_entry::~export_entry(void)
{
	if( library_name != NULL )
		delete [] library_name;
	if( name != NULL )
		delete [] name;
}

export_list::export_list()
{
	_min64 = _UI64_MAX;
	_max64 = UINT_MAX;
	_min32 = UINT_MAX;
	_max32 = 0;
	_bits32 = 0;
	_bits64 = 0;
}

bool export_list::contains(unsigned __int64 address)
{
	// Look up a 64-bit value
	if ( address <= UINT_MAX )
		return contains((unsigned __int32)address);
	
	if (address > _max64 || address < _min64 || (address & ~_bits64) > 0)
	{
		// We know there is no match by this quick filtering. This improves performance hugely.
		return false;
	}

	// Lookup the address
	unordered_set<unsigned __int64>::const_iterator got = _addresses.find(address);
	if (got != _addresses.end())
	{
		return true;
	}
	return false;
}

bool export_list::contains(unsigned __int32 address)
{
	// Look up a 32-bit value
	if (address > _max32 || address < _min32 || (address & ~_bits32) > 0)
	{
		// We know there is no match by this quick filtering. This improves performance hugely.
		return false;
	}

	// Lookup the address
	unordered_set<unsigned __int64>::const_iterator got = _addresses.find(address);
	if (got != _addresses.end())
	{
		return true;
	}
	return false;
}

unsigned __int64 export_list::find_export(char* library, char* name, bool is64)
{
	if (name == NULL)
		return 0;

	// Find the specified procedure in the corresponding library. Limit it to the specific 32-bit or 64-bit version of the library.
	for (unordered_map<unsigned __int64, export_entry*>::iterator it = _address_to_exports.begin(); it != _address_to_exports.end(); ++it)
	{
		//if( strcmp(it->second->library_name,"kernel32.dll") == 0 )
		//	printf("%s::%s\n", it->second->library_name, it->second->name);
		if (it->second->is64 == is64 && it->second->name != NULL &&
			(library == NULL || (it->second->library_name != NULL && strcmpi(library, it->second->library_name) == 0)) &&
			strcmpi(name, it->second->name) == 0)
		{
			// Found a match
			return it->second->address;
		}
	}

	// No match
	return 0;
}

export_entry export_list::find(unsigned __int64 address)
{
	// Lookup the address
	unordered_map<unsigned __int64, export_entry*>::const_iterator got = _address_to_exports.find(address);
	if (got != _address_to_exports.end())
	{
		return got->second;
	}
	return export_entry(NULL, NULL, 0, 0, 0, false);
}

void export_list::add_export(unsigned __int64 address, export_entry* entry)
{
	// Register this export address for quick lookups later
	if (_address_to_exports.count(address) == 0)
	{
		_address_to_exports.insert(std::pair<unsigned __int64, export_entry*>(address, new export_entry(entry)));

		if (_addresses.count(address) == 0)
		{
			_addresses.insert(address);

			// Update our quick-lookup values
			if ( address > UINT_MAX )
			{
				// 64bit value
				if (address > _max64)
					_max64 = address;
				if (address < _min64)
					_min64 = address;
				_bits64 = _bits64 | address;
			}
			else
			{
				// 32bit value
				if (address > _max32)
					_max32 = address;
				if (address < _min32)
					_min32 = address;
				_bits32 = _bits32 | address;
			}
		}
	}
}

bool export_list::add_exports(export_list* other)
{
	// Merge the exports from the other list with the current export list
	for (unordered_map<unsigned __int64,export_entry*>::iterator it = other->_address_to_exports.begin();
        it != other->_address_to_exports.end(); ++it) 
	{
		add_export(it->first, it->second);
	}
	return true;
}


namespace
{
	bool export_array_fits(SIZE_T image_size, DWORD offset, DWORD count, SIZE_T element_size)
	{
		return range_fits(image_size, offset, 0) &&
			count <= (image_size - offset) / element_size;
	}

	bool export_string(unsigned char* image, SIZE_T image_size, DWORD offset, std::string& value)
	{
		if (!range_fits(image_size, offset, 1))
			return false;
		const char* begin = reinterpret_cast<const char*>(image + offset);
		const char* end = static_cast<const char*>(memchr(begin, 0, image_size - offset));
		if (end == NULL || end == begin)
			return false;
		value.assign(begin, static_cast<SIZE_T>(end - begin));
		return true;
	}
}

bool export_list::add_exports(unsigned char* image, SIZE_T image_size, unsigned __int64 image_base, IMAGE_EXPORT_DIRECTORY* export_directory, bool is64)
{
	if (image == NULL || export_directory == NULL ||
		!test_read(image, image_size, reinterpret_cast<unsigned char*>(export_directory), sizeof(*export_directory)))
		return false;

	IMAGE_EXPORT_DIRECTORY directory;
	memcpy(&directory, export_directory, sizeof(directory));
	if (!export_array_fits(image_size, directory.AddressOfFunctions, directory.NumberOfFunctions, sizeof(DWORD)) ||
		!export_array_fits(image_size, directory.AddressOfNames, directory.NumberOfNames, sizeof(DWORD)) ||
		!export_array_fits(image_size, directory.AddressOfNameOrdinals, directory.NumberOfNames, sizeof(WORD)))
		return false;
	if (directory.NumberOfFunctions == 0)
		return directory.NumberOfNames == 0;
	if (directory.AddressOfFunctions == 0 ||
		(directory.NumberOfNames != 0 && (directory.AddressOfNames == 0 || directory.AddressOfNameOrdinals == 0)))
		return false;

	std::string library_name;
	if (!export_string(image, image_size, directory.Name, library_name))
	{
		fprintf(stderr, "WARNING: Invalid library export directory module name at base 0x%llX.\n", image_base);
		return false;
	}

	for (DWORD i = 0; i < directory.NumberOfNames; ++i)
	{
		WORD ordinal_relative;
		DWORD name_offset;
		memcpy(&ordinal_relative, image + directory.AddressOfNameOrdinals + static_cast<SIZE_T>(i) * sizeof(WORD), sizeof(ordinal_relative));
		memcpy(&name_offset, image + directory.AddressOfNames + static_cast<SIZE_T>(i) * sizeof(DWORD), sizeof(name_offset));
		if (ordinal_relative >= directory.NumberOfFunctions || directory.Base > MAXWORD - ordinal_relative)
			return false;

		std::string name;
		if (!export_string(image, image_size, name_offset, name))
			return false;

		DWORD rva;
		memcpy(&rva, image + directory.AddressOfFunctions + static_cast<SIZE_T>(ordinal_relative) * sizeof(DWORD), sizeof(rva));
		if (image_base > _UI64_MAX - rva)
			return false;

		// Keep the existing filter used to avoid false matches during import repair.
		if (rva % 0x1000 != 0)
		{
			const unsigned __int64 address = image_base + rva;
			export_entry entry(library_name.c_str(), name.c_str(), static_cast<WORD>(directory.Base + ordinal_relative), rva, address, is64);
			add_export(address, &entry);
		}
	}
	return true;
}

export_list::~export_list(void)
{
	// Clean up the export list
	for (unordered_map<unsigned __int64,export_entry*>::iterator it = _address_to_exports.begin();
        it != _address_to_exports.end(); ++it) 
	{
		delete it->second;
	}
	_address_to_exports.clear();
}
