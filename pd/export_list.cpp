#include "StdAfx.h"
#include "export_list.h"


export_entry::export_entry(char* library_name, char* name, WORD ord, __int64 rva, __int64 address)
{
	// Copy the strings locally, knowing the function might not have a name but has to have a library name
	this->library_name = new char[strlen(library_name) +1];
	strcpy( this->library_name, library_name );

	if( name != NULL )
	{
		this->name = new char[strlen(name) + 1];
		strcpy( this->name, name );
	}
	else
	{
		this->name = NULL;
	}

	this->ord = ord;
	this->rva = rva;
	this->address = address;
}

export_entry::export_entry(export_entry* other)
{
	// Copy the strings locally, knowing the function might not have a name but has to have a library name
	this->library_name = new char[strlen(other->library_name) +1];
	strcpy( this->library_name, other->library_name );
	
	if( other->name != NULL )
	{
		this->name = new char[strlen(other->name) + 1];
		strcpy( this->name, other->name );
	}
	else
	{
		this->name = NULL;
	}

	this->ord = other->ord;
	this->rva = other->rva;
	this->address = other->address;
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
}

bool export_list::add_exports(export_list* other)
{
	// Merge the exports from the other list with the current export list
	for (unordered_map<__int64,export_entry*>::iterator it = other->address_to_exports.begin(); 
        it != other->address_to_exports.end(); ++it) 
	{
		if( address_to_exports.count( it->first ) == 0 )
		{
			address_to_exports.insert( std::make_pair<__int64,export_entry*>(it->first, new export_entry(it->second) ) );
		}
	}
	return true;
}


bool export_list::add_exports(unsigned char* image, SIZE_T image_size, __int64 image_base, IMAGE_EXPORT_DIRECTORY* export_directory)
{
	if( export_directory->NumberOfFunctions > 0 && export_directory->AddressOfNameOrdinals != 0 )
	{
		char* library_name = (char*) ((__int64) export_directory->Name + image);
		if( !test_read(image, image_size, (unsigned char*) library_name, 0x1ff) || strlen(library_name) == 0 )
		{
			// Library name is invalid, no point in continuing to parse since we need this name to reconstruct any imports anyway
			//throw std::invalid_argument( printf("Invalid library export directory module name. Unable to add exports to table for import reconstruction. Library base 0x%llX.", (unsigned long long int) image_base ) );
			fprintf( stderr, "WARNING: Invalid library export directory module name. Unable to add exports to table for import reconstruction. Library base 0x%llX.\n",
											(unsigned long long int) image_base );
			return false;
		}
		
		// Parse the export directory
		for (int i = 0; i < export_directory->NumberOfNames; i++)
		{
			// Load the ordinal
			if( test_read(image, image_size, image + export_directory->AddressOfNameOrdinals + i*2, 2) )
			{
				DWORD ordinal_relative = *(WORD*)(export_directory->AddressOfNameOrdinals + i*2 + image);
				DWORD ordinal = export_directory->Base + ordinal_relative;
				
				// Load the name, there doesn't have to be one
				char* name = NULL;
				if (i < export_directory->NumberOfNames)
				{
					if( test_read(image, image_size, image + export_directory->AddressOfNames + i*4, 4) )
					{
						DWORD name_offset = *((DWORD*) (image + export_directory->AddressOfNames + i*4));
						
						if( test_read(image, image_size, image + name_offset, 0x4f) )
						{
							name = (char*) ( name_offset + image );
						}
					}
				}
				

				// Load the rva
				if( test_read(image, image_size, image + export_directory->AddressOfFunctions + ordinal_relative*4, 4) )
				{
					__int64 rva = *((DWORD*)(image + export_directory->AddressOfFunctions + ordinal_relative * 4));

					// Don't consider rva's of multiple of 0x1000 to prevent making mistakes in dump repairs
					if( rva % 0x1000 != 0 )
					{
						__int64 address = image_base + rva;
						

						
						// Add this export
						export_entry* new_entry = new export_entry( library_name, name, ordinal, rva, address );
						
						// Register this export address for quick lookups later
						if( address_to_exports.count( address ) == 0 )
						{
							address_to_exports.insert( std::make_pair<__int64,export_entry*>(address, new_entry) );
						}
					}
				}
				
			}
		}
	}
	return true;
	
	
}

export_list::~export_list(void)
{
	// Clean up the export list
	for (unordered_map<__int64,export_entry*>::iterator it = address_to_exports.begin(); 
        it != address_to_exports.end(); ++it) 
	{
		delete it->second;
	}
	address_to_exports.clear();
}
