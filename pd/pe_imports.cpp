#include "StdAfx.h"
#include "pe_imports.h"

void pe_imports::add_fixup(char* library_name, int ordinal, __int64 rva, bool win64)
{
	this->_libraries.Add(new import_library(library_name, ordinal, rva, win64));
}

void pe_imports::add_fixup(char* library_name, char* proc_name, __int64 rva, bool win64)
{
	this->_libraries.Add(new import_library(library_name, proc_name, rva, win64));
}


void pe_imports::get_table_size(__int64 &descriptor_size, __int64 &extra_size)
{
	for( int i = 0; i < _libraries.GetSize(); i++ )
	{
		_libraries[i]->get_table_size( descriptor_size, extra_size );
	}
	descriptor_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
}

bool pe_imports::build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 descriptor_offset, __int64 extra_offset)
{
	// Build the import table in the new section
	bool retval = true;
	for( int i = 0; i < _libraries.GetSize(); i++ )
	{
		if( !_libraries[i]->build_table(section, section_size, section_rva, descriptor_offset, extra_offset) )
			retval = false;
	}

	// Write the final descriptor
	IMAGE_IMPORT_DESCRIPTOR blank_descrptor;
	memset( &blank_descrptor, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR) );

	if( test_read( section, section_size, section + descriptor_offset , sizeof(IMAGE_IMPORT_DESCRIPTOR) ) )
		memcpy( section + descriptor_offset, &blank_descrptor, sizeof(IMAGE_IMPORT_DESCRIPTOR) );


	return retval;
}

pe_imports::pe_imports(unsigned char* image, __int64 image_size, IMAGE_IMPORT_DESCRIPTOR* imports, bool win64)
{
	_win64 = win64;

	// Process the import descriptor directory
	bool more;
	int i = 0;
	do
	{
		more = false;
		if( test_read( image, image_size, ((unsigned char*)imports) + i * sizeof(IMAGE_IMPORT_DESCRIPTOR ),
			sizeof(IMAGE_IMPORT_DESCRIPTOR ) ) )
		{
			IMAGE_IMPORT_DESCRIPTOR* current = &((IMAGE_IMPORT_DESCRIPTOR*) imports)[i];
			if( current->Characteristics != 0 || current->FirstThunk != 0 || current->ForwarderChain != 0 || current->Name != 0 )
			{
				this->add_descriptor(current);
				more = true;
				i++;
			}
		}
	}while(more);
}

void pe_imports::add_descriptor(IMAGE_IMPORT_DESCRIPTOR* descriptor)
{
	this->_libraries.Add( new import_library(descriptor, _win64) );
}

pe_imports::~pe_imports(void)
{
}

import_library::~import_library(void)
{
	delete _descriptor;

	if( _import_by_name != NULL )
		delete[] _import_by_name;

	if( _library_name != NULL )
		delete[] _library_name;

	if( _thunk_entry != NULL )
		delete _thunk_entry;
}

import_library::import_library(IMAGE_IMPORT_DESCRIPTOR* descriptor, bool win64)
{
	// Localize the descriptor, TODO: and the referenced data maybe
	_descriptor = new IMAGE_IMPORT_DESCRIPTOR(*descriptor);
	_import_by_name = NULL;
	_library_name = NULL;
	_thunk_entry = NULL;
}

import_library::import_library(char* library_name, int ordinal, __int64 rva, bool win64)
{
	// Create an import library to fixup a specific rva to the ordinal
	_descriptor = new IMAGE_IMPORT_DESCRIPTOR();
	_descriptor->OriginalFirstThunk = NULL; // Replace with rva to this->thunk_entry when outputting
	_descriptor->TimeDateStamp = -1;
	_descriptor->ForwarderChain = -1;
	_descriptor->Name = NULL; // Replace with rva to name upon writing
	_descriptor->FirstThunk = rva; // PE Loader with patchup address at rva to become the address of the import, awesome!
	_import_by_name = NULL;
	_thunk_entry = new IMAGE_THUNK_DATA64();

	_library_name = new char[strlen(library_name)+1];
	strcpy(_library_name, library_name);
	
	// Ordinal import
	if( win64 )
		_thunk_entry->u1.Ordinal = IMAGE_ORDINAL_FLAG64 | (ordinal & 0xffff);
	else
		_thunk_entry->u1.Ordinal = IMAGE_ORDINAL_FLAG32 | (ordinal & 0xffff);
}

import_library::import_library(char* library_name, char* proc_name, __int64 rva, bool win64)
{
	// Create an import library to fixup a specific rva to the ordinal
	_descriptor = new IMAGE_IMPORT_DESCRIPTOR();
	_descriptor->OriginalFirstThunk = NULL; // Replace with rva to this->thunk_entry when outputting
	_descriptor->TimeDateStamp = -1;
	_descriptor->ForwarderChain = -1;
	_descriptor->Name = NULL; // Replace with rva to name upon writing
	_descriptor->FirstThunk = rva; // PE Loader with patchup address at rva to become the address of the import, awesome!
	_thunk_entry = new IMAGE_THUNK_DATA64();

	_library_name = new char[strlen(library_name)+1];
	strcpy(_library_name, library_name);
	
	// Name import
	_thunk_entry->u1.AddressOfData = NULL; // Replace with rva to import_by_name
	
	_import_by_name = (IMAGE_IMPORT_BY_NAME*) new char[strlen(proc_name)+1+sizeof(WORD)]; // {DWORD, procedure name}
	_import_by_name->Hint = 0; // Not necessary
	_import_by_name_len = strlen(proc_name)+1+sizeof(WORD);
	strcpy((char*)(&_import_by_name->Name), proc_name);
}

void import_library::get_table_size(__int64 &descriptor_size, __int64 &extra_size)
{
	// Calculate the size required
	
	// Write the _import_by_name name if necessary
	if( _import_by_name != NULL )
	{
		extra_size += _import_by_name_len;
	}

	// Write the _thunk_entry IMAGE_THUNK_DATA64 struct, and update it
	if( _thunk_entry != NULL )
	{
		extra_size += 2 * sizeof(IMAGE_THUNK_DATA64);
	}

	// Patch up the library name
	if( _library_name != NULL )
	{
		extra_size += strlen(_library_name) + 1;
	}

	// Copy over the descriptor
	descriptor_size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
}

bool import_library::build_table(unsigned char* section, __int64 section_size, __int64 section_rva, __int64 &descriptor_offset, __int64 &extra_offset)
{
	// Output this import table into the specified section blob
	
	// Write the _import_by_name name if necessary
	__int64 import_name_rva = 0;
	if( _import_by_name != NULL )
	{
		if( !test_read(section, section_size, extra_offset + section, _import_by_name_len ) )
			return false; // Not enough room
		
		memcpy(extra_offset + section, (char*)_import_by_name, _import_by_name_len);
		import_name_rva = extra_offset + section_rva;
		extra_offset += _import_by_name_len;
	}

	// Write the _thunk_entry IMAGE_THUNK_DATA64 struct, and update it
	__int64 thunk_entry_rva = 0;
	if( _thunk_entry != NULL )
	{
		if( !test_read(section, section_size, extra_offset + section, sizeof(IMAGE_THUNK_DATA64)*2 ) )
			return false;
		
		if( import_name_rva != NULL )
			_thunk_entry->u1.AddressOfData = import_name_rva;
		
		memcpy(extra_offset + section, (char*) _thunk_entry, sizeof(IMAGE_THUNK_DATA64));
		thunk_entry_rva = extra_offset + section_rva;
		
		// Copy a null _thunk_entry terminator. For a 32 bit module, we are wasting a DWORD for the 64 structure, but that's fine.
		memset(extra_offset + section + sizeof(IMAGE_THUNK_DATA64), 0, sizeof(IMAGE_THUNK_DATA64));
		
		extra_offset += 2*sizeof(IMAGE_THUNK_DATA64);
	}
	
	// Update and write the IMAGE_IMPORT_DESCRIPTOR
	if( _descriptor == NULL )
		return false;
	if( !test_read(section, section_size, descriptor_offset + section, sizeof(IMAGE_IMPORT_DESCRIPTOR)) )
		return false; // Not enough room

	if( thunk_entry_rva != NULL )
		_descriptor->OriginalFirstThunk = thunk_entry_rva; // Redirect this to our thunk rva
	
	// Patch up the library name
	if( _library_name != NULL )
	{
		if( !test_read(section, section_size, extra_offset + section, strlen(_library_name) + 1) )
			return false; // Not enough room

		strcpy((char*)(extra_offset + section), _library_name);
		_descriptor->Name = extra_offset + section_rva;
		
		extra_offset += strlen(_library_name) + 1;
	}

	// Copy over the descriptor
	memcpy(descriptor_offset + section, (unsigned char*)_descriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR));
	descriptor_offset += sizeof(IMAGE_IMPORT_DESCRIPTOR);
	
	return true;
}