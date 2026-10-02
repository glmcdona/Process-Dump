#include "StdAfx.h"
#include "pe_imports.h"
#include <limits.h>

namespace
{
	bool bounded_size(__int64 size)
	{
		return size >= 0 && static_cast<unsigned __int64>(size) <= MAX_PE_IMAGE_SIZE;
	}

	bool table_range(__int64 size, __int64 offset, __int64 count)
	{
		return bounded_size(size) && bounded_size(offset) && bounded_size(count) &&
			range_fits(static_cast<SIZE_T>(size), static_cast<SIZE_T>(offset), static_cast<SIZE_T>(count));
	}

	bool table_rva(__int64 rva, __int64 offset, __int64 count)
	{
		return rva >= 0 && rva <= MAXDWORD && offset >= 0 && offset <= MAXDWORD - rva &&
			count >= 0 && count <= MAXDWORD - rva - offset;
	}

	void add_size(__int64& size, SIZE_T count)
	{
		if (!bounded_size(size) || count > MAX_PE_IMAGE_SIZE - static_cast<SIZE_T>(size))
			size = -1;
		else
			size += count;
	}
}

void pe_imports::_add(import_library library)
{
	__int64 descriptor_size = 0, extra_size = 0;
	library.get_table_size(descriptor_size, extra_size);
	const SIZE_T overhead = 2 * sizeof(import_library);
	if (!_valid || !library.valid() || !bounded_size(descriptor_size) || !bounded_size(extra_size) ||
		!table_range(MAX_PE_IMAGE_SIZE, descriptor_size, extra_size) ||
		!table_range(MAX_PE_IMAGE_SIZE, descriptor_size + extra_size, overhead) ||
		!table_range(MAX_PE_IMAGE_SIZE, _owned_size, descriptor_size + extra_size + overhead))
	{
		_valid = false;
		return;
	}
	_owned_size += static_cast<SIZE_T>(descriptor_size + extra_size) + overhead;
	if (_libraries.size() == _libraries.capacity())
		_libraries.reserve(_libraries.empty() ? 1 : 2 * _libraries.size());
	_libraries.push_back(std::move(library));
}

void pe_imports::add_fixup(char* library_name, int ordinal, __int64 rva, bool win64)
{
	if (_valid)
		_add(import_library(library_name, ordinal, rva, win64));
}

void pe_imports::add_fixup(char* library_name, char* proc_name, __int64 rva, bool win64)
{
	if (_valid)
		_add(import_library(library_name, proc_name, rva, win64));
}

void pe_imports::add_fixup(const export_entry& entry, __int64 rva, bool win64)
{
	if (_valid)
		_add(import_library(entry, rva, win64));
}

void pe_imports::get_table_size(__int64& descriptor_size, __int64& extra_size)
{
	if (!_valid)
	{
		descriptor_size = extra_size = -1;
		return;
	}
	for (auto& library : _libraries)
		library.get_table_size(descriptor_size, extra_size);
	add_size(descriptor_size, sizeof(IMAGE_IMPORT_DESCRIPTOR));
}

bool pe_imports::build_table(unsigned char* section, __int64 section_size, __int64 section_rva,
	__int64 descriptor_offset, __int64 extra_offset)
{
	__int64 descriptor_size = 0, extra_size = 0;
	get_table_size(descriptor_size, extra_size);
	if (!_valid || section == NULL ||
		!table_range(section_size, descriptor_offset, descriptor_size) ||
		!table_range(section_size, extra_offset, extra_size) ||
		descriptor_offset + descriptor_size > extra_offset ||
		!table_rva(section_rva, 0, section_size))
		return false;

	for (auto& library : _libraries)
	{
		if (!library.build_table(section, section_size, section_rva, descriptor_offset, extra_offset))
			return false;
	}
	memset(section + static_cast<SIZE_T>(descriptor_offset), 0, sizeof(IMAGE_IMPORT_DESCRIPTOR));
	return true;
}

pe_imports::pe_imports(unsigned char* image, __int64 image_size, IMAGE_IMPORT_DESCRIPTOR* imports, bool win64)
	: _win64(win64)
{
	if (!bounded_size(image_size))
	{
		_valid = false;
		return;
	}
	if (imports == NULL)
		return;
	if (!test_read(image, static_cast<SIZE_T>(image_size), reinterpret_cast<unsigned char*>(imports),
		sizeof(IMAGE_IMPORT_DESCRIPTOR)))
	{
		_valid = false;
		return;
	}
	SIZE_T offset = reinterpret_cast<ULONG_PTR>(imports) - reinterpret_cast<ULONG_PTR>(image);
	while (range_fits(static_cast<SIZE_T>(image_size), offset, sizeof(IMAGE_IMPORT_DESCRIPTOR)))
	{
		IMAGE_IMPORT_DESCRIPTOR* current = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image + offset);
		if (current->Characteristics == 0 && current->FirstThunk == 0 &&
			current->ForwarderChain == 0 && current->Name == 0)
			return;
		add_descriptor(current);
		if (!_valid)
			return;
		offset += sizeof(IMAGE_IMPORT_DESCRIPTOR);
	}
	_valid = false;
}

void pe_imports::add_descriptor(IMAGE_IMPORT_DESCRIPTOR* descriptor)
{
	if (_valid)
		_add(import_library(descriptor, _win64));
}

pe_imports::~pe_imports(void) = default;

import_library::import_library(IMAGE_IMPORT_DESCRIPTOR* descriptor, bool win64)
{
	if (descriptor != NULL)
	{
		_descriptor = *descriptor;
		_valid = true;
	}
}

import_library::import_library(char* library_name, int ordinal, __int64 rva, bool win64)
{
	_initialize(library_name, NULL, ordinal, rva, win64);
	_copy_names();
}

import_library::import_library(char* library_name, char* proc_name, __int64 rva, bool win64)
{
	if (proc_name != NULL)
		_initialize(library_name, proc_name, 0, rva, win64);
	_copy_names();
}

import_library::import_library(const export_entry& entry, __int64 rva, bool win64)
{
	_initialize(entry.library_name, entry.name, entry.ord, rva, win64);
}

void import_library::_initialize(const char* library_name, const char* proc_name, int ordinal, __int64 rva, bool win64)
{
	if (library_name == NULL || rva < 0 || rva > MAXDWORD)
		return;
	const SIZE_T length = strnlen_s(library_name, MAX_PE_IMAGE_SIZE);
	if (length >= MAX_PE_IMAGE_SIZE)
		return;
	_library_name_len = static_cast<DWORD>(length + 1);
	_library_name = library_name;
	if (proc_name != NULL)
	{
		const SIZE_T proc_length = strnlen_s(proc_name, MAX_PE_IMAGE_SIZE);
		if (proc_length > MAX_PE_IMAGE_SIZE - sizeof(WORD) - 1 ||
			!range_fits(MAX_PE_IMAGE_SIZE, _library_name_len, proc_length + 1))
			return;
		_proc_name_len = static_cast<DWORD>(proc_length + 1);
		_proc_name = proc_name;
	}
	_descriptor.TimeDateStamp = MAXDWORD;
	_descriptor.ForwarderChain = MAXDWORD;
	_descriptor.FirstThunk = static_cast<DWORD>(rva);
	_thunk_entry.u1.Ordinal = proc_name == NULL ?
		(win64 ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) | (ordinal & 0xffff) : 0;
	_valid = true;
}

void import_library::_copy_names()
{
	if (!_valid)
		return;
	_owned_names.reset(new char[static_cast<SIZE_T>(_library_name_len) + _proc_name_len]);
	memcpy(_owned_names.get(), _library_name, _library_name_len);
	_library_name = _owned_names.get();
	if (_proc_name != NULL)
	{
		memcpy(_owned_names.get() + _library_name_len, _proc_name, _proc_name_len);
		_proc_name = _owned_names.get() + _library_name_len;
	}
}

void import_library::get_table_size(__int64& descriptor_size, __int64& extra_size)
{
	if (!valid())
	{
		descriptor_size = extra_size = -1;
		return;
	}
	if (_proc_name != NULL)
		add_size(extra_size, sizeof(WORD) + _proc_name_len);
	if (_library_name != NULL)
		add_size(extra_size, 2 * sizeof(IMAGE_THUNK_DATA64));
	add_size(extra_size, _library_name_len);
	add_size(descriptor_size, sizeof(IMAGE_IMPORT_DESCRIPTOR));
}

bool import_library::build_table(unsigned char* section, __int64 section_size, __int64 section_rva,
	__int64& descriptor_offset, __int64& extra_offset)
{
	__int64 descriptor_size = 0, extra_size = 0;
	get_table_size(descriptor_size, extra_size);
	if (!valid() || section == NULL ||
		!table_range(section_size, descriptor_offset, descriptor_size) ||
		!table_range(section_size, extra_offset, extra_size) ||
		descriptor_offset + descriptor_size > extra_offset ||
		!table_rva(section_rva, 0, section_size))
		return false;

	IMAGE_IMPORT_DESCRIPTOR descriptor = _descriptor;
	__int64 import_name_rva = 0;
	if (_proc_name != NULL)
	{
		memset(section + static_cast<SIZE_T>(extra_offset), 0, sizeof(WORD));
		memcpy(section + static_cast<SIZE_T>(extra_offset) + sizeof(WORD), _proc_name, _proc_name_len);
		import_name_rva = section_rva + extra_offset;
		extra_offset += sizeof(WORD) + _proc_name_len;
	}
	if (_library_name != NULL)
	{
		IMAGE_THUNK_DATA64 thunk = _thunk_entry;
		if (import_name_rva != 0)
			thunk.u1.AddressOfData = import_name_rva;
		memcpy(section + static_cast<SIZE_T>(extra_offset), &thunk, sizeof(thunk));
		memset(section + static_cast<SIZE_T>(extra_offset) + sizeof(thunk), 0, sizeof(thunk));
		descriptor.OriginalFirstThunk = static_cast<DWORD>(section_rva + extra_offset);
		extra_offset += 2 * sizeof(thunk);
	}
	if (_library_name != NULL)
	{
		memcpy(section + static_cast<SIZE_T>(extra_offset), _library_name, _library_name_len);
		descriptor.Name = static_cast<DWORD>(section_rva + extra_offset);
		extra_offset += _library_name_len;
	}
	memcpy(section + static_cast<SIZE_T>(descriptor_offset), &descriptor, sizeof(descriptor));
	descriptor_offset += sizeof(descriptor);
	return true;
}
