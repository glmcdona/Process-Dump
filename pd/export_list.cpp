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
	return lookup(address) != NULL;
}

bool export_list::contains(unsigned __int32 address)
{
	return lookup(address) != NULL;
}

const export_entry* export_list::lookup(unsigned __int64 address) const
{
	if (address <= UINT_MAX)
	{
		if (address > _max32 || address < _min32 || (address & ~_bits32) != 0)
			return NULL;
	}
	else if (address > _max64 || address < _min64 || (address & ~_bits64) != 0)
		return NULL;
	const auto found = _address_to_exports.find(address);
	return found == _address_to_exports.end() ? NULL : found->second;
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
		_add_owned(address, std::unique_ptr<export_entry>(new export_entry(entry)));
}

void export_list::_add_owned(unsigned __int64 address, std::unique_ptr<export_entry> entry)
{
	if (_address_to_exports.emplace(address, entry.get()).second)
	{
		entry.release();
		_update_filters(address);
	}
}

void export_list::_update_filters(unsigned __int64 address)
{
	if (address > UINT_MAX)
	{
		_max64 = (std::max)(_max64, address);
		_min64 = (std::min)(_min64, address);
		_bits64 |= address;
	}
	else
	{
		_max32 = (std::max)(_max32, static_cast<unsigned __int32>(address));
		_min32 = (std::min)(_min32, static_cast<unsigned __int32>(address));
		_bits32 |= static_cast<unsigned __int32>(address);
	}
}

void export_list::take_exports(export_list& other)
{
	if (&other == this)
		return;
	for (auto entry = other._address_to_exports.begin(); entry != other._address_to_exports.end();)
	{
		if (_address_to_exports.emplace(entry->first, entry->second).second)
			_update_filters(entry->first);
		else
			delete entry->second;
		entry = other._address_to_exports.erase(entry);
	}
	other._min64 = _UI64_MAX;
	other._max64 = UINT_MAX;
	other._min32 = UINT_MAX;
	other._max32 = 0;
	other._bits32 = 0;
	other._bits64 = 0;
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

bool export_list::add_exports(unsigned char* image, SIZE_T image_size, unsigned __int64 image_base, IMAGE_EXPORT_DIRECTORY* export_directory, bool is64, DWORD directory_size)
{
	if (image == NULL || export_directory == NULL ||
		!test_read(image, image_size, reinterpret_cast<unsigned char*>(export_directory), sizeof(*export_directory)))
		return false;

	IMAGE_EXPORT_DIRECTORY directory;
	memcpy(&directory, export_directory, sizeof(directory));
	const SIZE_T directory_rva = reinterpret_cast<unsigned char*>(export_directory) - image;
	if (directory_size != 0 && (directory_size < sizeof(directory) || !range_fits(image_size, directory_rva, directory_size)))
		return false;
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

	export_list parsed;
	const auto add_function = [&](DWORD index, const char* name) {
		DWORD rva;
		memcpy(&rva, image + directory.AddressOfFunctions + static_cast<SIZE_T>(index) * sizeof(DWORD), sizeof(rva));
		if (rva == 0)
			return true;
		// EAT entries inside the export directory name a forwarder, not executable/data addresses.
		if (directory_size != 0 && rva >= directory_rva && rva - directory_rva < directory_size)
			return true;
		// Loaded export tables can redirect functions outside their own module (for example WOW64 USER32).
		if (image_base > _UI64_MAX - rva || index > MAXWORD || directory.Base > MAXWORD - index)
			return false;
		const auto address = image_base + rva;
		if (!parsed.contains(address))
			parsed._add_owned(address, std::unique_ptr<export_entry>(new export_entry(library_name.c_str(),
				name, static_cast<WORD>(directory.Base + index), rva, address, is64)));
		return true;
	};
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

		if (!add_function(ordinal_relative, name.c_str()))
			return false;
	}
	// Named entries win aliases at the same address; all remaining EAT entries are ordinal imports.
	for (DWORD i = 0; i < directory.NumberOfFunctions; ++i)
		if (!add_function(i, NULL))
			return false;
	take_exports(parsed);
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
