#include "StdAfx.h"
#include "pe_header.h"

pe_header::pe_header( char* filename, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	this->_stream = (stream_wrapper*) new file_stream( filename );
	_original_base = 0;
	_unique_hash = 0;

	_name_filepath_long_size = 0;
	_name_filepath_long = NULL;
	_name_filepath_short_size = 0;
	_name_filepath_short = NULL;
	_name_original_exports_size = 0;
	_name_original_exports = NULL;
	_name_original_manifest_size = 0;
	_name_original_manifest = NULL;
	_name_symbols_path_size = 0;
	_name_symbols_path = NULL;
	_export_list = NULL;

	this->_parsed_dos = false;
	this->_parsed_pe_32 = false;
	this->_parsed_pe_64 = false;
	this->_parsed_sections = false;
	this->_image_size = 0;
	this->_disk_image_size = 0;
	this->_unique_hash = 0;

	if( _stream != NULL )
	{
		// Assign the disk filename for this file
		_name_filepath_long = new char[FILEPATH_SIZE];
		_name_filepath_long_size = _stream->get_long_name( _name_filepath_long, FILEPATH_SIZE );
		_name_filepath_short = new char[FILEPATH_SIZE];
		_name_filepath_short_size = _stream->get_short_name( _name_filepath_short, FILEPATH_SIZE );
	}

	if( _options->Verbose )
		fprintf( stdout, "INFO: Initialized header for module name %s.\r\n", this->get_name() );
}

export_list* pe_header::get_exports()
{
	if( (_parsed_pe_32 || _parsed_pe_64) && _export_list != NULL )
	{
		return this->_export_list;
	}
	return NULL;
}

pe_header::pe_header( DWORD pid, void* base, module_list* modules, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	_unique_hash = 0;

	_header_export_directory = NULL;
	_header_import_descriptors = NULL;
	_name_filepath_long_size = 0;
	_name_filepath_long = NULL;
	_name_filepath_short_size = 0;
	_name_filepath_short = NULL;
	_name_original_exports_size = 0;
	_name_original_exports = NULL;
	_name_original_manifest_size = 0;
	_name_original_manifest = NULL;
	_name_symbols_path_size = 0;
	_name_symbols_path = NULL;
	_export_list = NULL;

	this->_parsed_dos = false;
	this->_parsed_pe_32 = false;
	this->_parsed_pe_64 = false;
	this->_parsed_sections = false;
	this->_image_size = 0;
	this->_disk_image_size = 0;
	this->_unique_hash = 0;

	this->_stream = (stream_wrapper*) new process_stream( pid, base, modules );
	_original_base = base;

	if( _stream != NULL )
	{
		// Assign the disk filename for this file
		_name_filepath_long = new char[FILEPATH_SIZE];
		_name_filepath_long_size = _stream->get_long_name( _name_filepath_long, FILEPATH_SIZE );
		_name_filepath_short = new char[FILEPATH_SIZE];
		_name_filepath_short_size = _stream->get_short_name( _name_filepath_short, FILEPATH_SIZE );
	}

	if( _options->Verbose )
		fprintf( stdout, "INFO: Initialized header for module name %s.\r\n", this->get_name() );
}

pe_header::pe_header( DWORD pid, module_list* modules, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	_unique_hash = 0;

	_header_export_directory = NULL;
	_header_import_descriptors = NULL;
	_name_filepath_long_size = 0;
	_name_filepath_long = NULL;
	_name_filepath_short_size = 0;
	_name_filepath_short = NULL;
	_name_original_exports_size = 0;
	_name_original_exports = NULL;
	_name_original_manifest_size = 0;
	_name_original_manifest = NULL;
	_name_symbols_path_size = 0;
	_name_symbols_path = NULL;
	_export_list = NULL;

	this->_parsed_dos = false;
	this->_parsed_pe_32 = false;
	this->_parsed_pe_64 = false;
	this->_parsed_sections = false;
	this->_image_size = 0;
	this->_disk_image_size = 0;
	this->_unique_hash = 0;

	this->_stream = (stream_wrapper*) new process_stream( pid, modules );
	_original_base = ((process_stream*) _stream)->base;

	if( _options->Verbose )
		fprintf( stdout, "INFO: Initialized header for module name %s.\r\n", this->get_name() );
}

pe_header::pe_header( HANDLE ph, void* base, module_list* modules, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	_unique_hash = 0;

	_name_filepath_long_size = 0;
	_name_filepath_long = NULL;
	_name_filepath_short_size = 0;
	_name_filepath_short = NULL;
	_name_original_exports_size = 0;
	_name_original_exports = NULL;
	_name_original_manifest_size = 0;
	_name_original_manifest = NULL;
	_name_symbols_path_size = 0;
	_name_symbols_path = NULL;
	_export_list = NULL;

	this->_parsed_dos = false;
	this->_parsed_pe_32 = false;
	this->_parsed_pe_64 = false;
	this->_parsed_sections = false;
	this->_image_size = 0;
	this->_disk_image_size = 0;
	this->_unique_hash = 0;

	this->_stream = (stream_wrapper*) new process_stream( ph, base );
	_original_base = base;

	if( _options->Verbose )
		fprintf( stdout, "INFO: Initialized header for module name %s.\r\n", this->get_name() );
}

void pe_header::print_report(FILE* stream)
{
	// Print the on-disk filepath if there is an associated file

	// Print the original filename specified by the exports table if it has one

	// Print the original filename specified by the manifest file if it has one

	// Print the symbols .pdb file path and name if found
	
	// Print the basic information

}

bool pe_header::somewhat_parsed()
{
	return _parsed_pe_32 || _parsed_pe_64;
}

bool pe_header::is_dll()
{
	if( this->_parsed_pe_32 )
		return (this->_header_pe32->FileHeader.Characteristics & IMAGE_FILE_DLL);
	if( this->_parsed_pe_64 )
		return (this->_header_pe64->FileHeader.Characteristics & IMAGE_FILE_DLL);
	return false;
}

bool pe_header::is_exe()
{
	if( this->_parsed_pe_32 )
		return !(this->_header_pe32->FileHeader.Characteristics & IMAGE_FILE_DLL) && !(this->_header_pe32->FileHeader.Characteristics & IMAGE_FILE_SYSTEM);
	if( this->_parsed_pe_64 )
		return !(this->_header_pe64->FileHeader.Characteristics & IMAGE_FILE_DLL) && !(this->_header_pe64->FileHeader.Characteristics & IMAGE_FILE_SYSTEM);
	return false;
}

bool pe_header::is_sys()
{
	if( this->_parsed_pe_32 )
		return this->_header_pe32->FileHeader.Characteristics & IMAGE_FILE_SYSTEM;
	if( this->_parsed_pe_64 )
		return this->_header_pe64->FileHeader.Characteristics & IMAGE_FILE_SYSTEM;
	return false;
}

bool pe_header::is_64()
{
	return this->_parsed_pe_64;
}

void pe_header::set_name(char* new_name)
{
	// Set name to sue for this module
	if( _name_filepath_short != NULL )
		delete _name_filepath_short;

	// Localize
	_name_filepath_short = new char[strlen(new_name) + 1];
	strcpy_s(_name_filepath_short, strlen(new_name) + 1, new_name);
	_name_filepath_short_size = strlen(_name_filepath_short);
}

char* pe_header::get_name()
{
	// Return the name of this module if available.
	if( this->_name_filepath_short_size > 0 && _name_filepath_short != NULL )
		return _name_filepath_short;
	return "hiddenmodule";
}

unsigned __int64 pe_header::get_virtual_size()
{
	if( this->_parsed_pe_32 || this->_parsed_pe_64 )
	{
		return _image_size;
	}
	return 0;
}

bool pe_header::process_hash( )
{
	// Build the hash of this library if has been loaded
	this->_unique_hash = 0;
	if( this->_parsed_pe_32 || this->_parsed_pe_64 )
	{
		// Hash the PE directory up until the end of the section definition, and hashes
		// this with the length of the import table, and number of modules in the import
		// table

		// First calculate begin hashing from the import table entries
		SIZE_T offset = 0;
		SIZE_T read_size = 0;
		if( _parsed_pe_32 )
		{
			offset = _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
			read_size = 4;
		}
		else
		{
			offset = _header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
			read_size = 8;
		}

		unsigned __int64 last_dw = -1;

		// Hash the IAT overview (not the exact values, just the structure).
		bool more;
		do
		{
			more = false;
			if( _test_read( _image, _image_size, _image + offset, read_size ) )
			{
				unsigned __int64 new_dw;
				if( read_size == 4 )
					new_dw = *((DWORD*) (_image + (long) offset));
				if( read_size == 8 )
					new_dw = *((unsigned __int64*) (_image + (long) offset));

				if( new_dw == 0 && last_dw == 0 )
					break;
				if( new_dw == 0 )
				{
					// New module hash
					_unique_hash = _unique_hash ^ 0x8ADFA91F8ADFA91F;
					_unique_hash = _rotl64(_unique_hash, 0x13);
				}
				else
				{
					// New import in module hash
					_unique_hash = _unique_hash ^ 0x18F31A228FA9B17A;
					_unique_hash = _rotl64(_unique_hash, 0x17);
				}
				offset += read_size;
				last_dw = new_dw;
				more = true;
			}
		}while(more);
		
		/*
		// Hash this with the DOS header
		unsigned char* start = 0;
		SIZE_T length = 0;
		
		start = (unsigned char*) &_header_dos;
		length = sizeof(IMAGE_DOS_HEADER);
		for( unsigned char* i = start; i + 8 < start + length; i+= 4 )
		{
			// Hash with this segment
			_unique_hash = _unique_hash ^ *((unsigned __int32*)(i));
			_unique_hash = _rotl64(_unique_hash, 0x19);
		}
		*/


		// Hash this with some of the section information
		
		if( this->_parsed_sections )
		{
			for( int i = 0; i < this->_num_sections; i++ )
			{
				// Hash with this section description
				_unique_hash = _unique_hash ^ *((unsigned __int64*)(&_header_sections[i].Name));
				_unique_hash = _rotl64(_unique_hash, 0x21);
				_unique_hash = _unique_hash ^ _header_sections[i].SizeOfRawData;
				_unique_hash = _rotl64(_unique_hash, 0x13);
				_unique_hash = _unique_hash ^ _header_sections[i].Characteristics;
				_unique_hash = _rotl64(_unique_hash, 0x17);
			}
		}
		
		
		return true;
	}

	return false;
	
	return true;
}

bool pe_header::write_image( char* filename )
{
	// Writes the loaded and reconstructed memory image to a file
	if( _disk_image_size > 0 )
	{
		FILE* fh = fopen( filename, "wb" );
		if( fh != NULL )
		{
			// Write the image
			fwrite( _disk_image, 1, _disk_image_size, fh );
			fclose(fh);
		}
	}
	return false;
}

IMPORT_SUMMARY pe_header::get_imports_information( export_list* exports )
{
	return get_imports_information( exports, _image_size );
}

IMPORT_SUMMARY pe_header::get_imports_information( export_list* exports, __int64 size_limit )
{
	// Builds a structure of information about the imports declared by this PE object. This includes:
	//   # of different import addresses
	//	 # of code locations that imported
	//	 Generic import hash
	//   Specific import hash

	// Gets the number of distinct import addresses that are imported.
	unordered_set<unsigned __int64> import_addresses;

	if( _options->Verbose )
			printf( "INFO: Building import information.\r\n" );
	
	IMPORT_SUMMARY result;
	result.COUNT_UNIQUE_IMPORT_ADDRESSES = 0;
	result.COUNT_UNIQUE_IMPORT_LIBRARIES = 0;
	result.HASH_GENERIC = 0;
	result.HASH_SPECIFIC = 0;

	size_t hash_generic = 0x1a78ac10;
	size_t hash_specific = 0x1a78ac10;

	hash<string> hasher;
	
	if( this->_parsed_sections )
	{
		// Add matches to exports in this process
		unsigned __int32 cand32_last = 0;
		unsigned __int64 cand64_last = 0;
		for(__int64 offset = 0; offset < _image_size - 8 && offset < size_limit - 8; offset+=4 )
		{
			// Check if this 4-gram or 8-gram points to an export
			unsigned __int32 cand32 = *((__int32*)(_image + offset));

			if (cand32 != cand32_last)
			{
				if (exports->contains(cand32))
				{
					export_entry entry = exports->find(cand32);

					// Found an import reference
					unordered_set<unsigned __int64>::const_iterator gotImportAddress = import_addresses.find(cand32);

					if (gotImportAddress == import_addresses.end())
					{
						// Add this new import
						import_addresses.insert(cand32);
						result.COUNT_UNIQUE_IMPORT_ADDRESSES++;

						// Add this imported function hash
						if (entry.name != NULL)
						{
							hash_generic = hash_generic ^ hasher(string(entry.name));
							hash_specific = hash_specific ^ hasher(string(entry.name));
						}
						if (entry.library_name != NULL)
						{
							hash_generic = hash_generic ^ (hasher(string(entry.library_name)) << 1);
							hash_specific = hash_specific ^ (hasher(string(entry.library_name)) << 1);
						}
						hash_generic = _rotl(hash_generic, 0x05);
						hash_specific = hash_specific ^ offset;
						hash_specific = _rotl(hash_specific, 0x05);
					}
				}
			}
			cand32_last = cand32;
			
			unsigned __int64 cand64 = *((unsigned __int64*)(_image + offset));
			if (cand64 != cand64_last && cand64 > 0xffffffff)
			{
				if (exports->contains(cand64))
				{
					export_entry entry = exports->find(cand64);

					// Found an import reference
					unordered_set<unsigned __int64>::const_iterator gotImportAddress = import_addresses.find(cand64);

					if (gotImportAddress == import_addresses.end())
					{
						// Add this new import
						import_addresses.insert(cand64);
						result.COUNT_UNIQUE_IMPORT_ADDRESSES++;

						// Add this imported function hash
						if (entry.name != NULL)
						{
							hash_generic = hash_generic ^ hasher(string(entry.name));
							hash_specific = hash_specific ^ hasher(string(entry.name));
						}
						if (entry.library_name != NULL)
						{
							hash_generic = hash_generic ^ (hasher(string(entry.library_name)) << 1);
							hash_specific = hash_specific ^ (hasher(string(entry.library_name)) << 1);
						}
						hash_generic = _rotl(hash_generic, 0x05);
						hash_specific = hash_specific ^ offset;
						hash_specific = _rotl(hash_specific, 0x05);
					}
				}
			}
			cand64_last = cand64;
		}
	}
	
	result.HASH_GENERIC = hash_generic;
	result.HASH_SPECIFIC = hash_specific;

	if( _options->Verbose )
	{
		printf( "INFO: Finished building import information:\r\n" );
		printf( "INFO: Count Unique Import Addresses = %i\r\n", result.COUNT_UNIQUE_IMPORT_ADDRESSES );
		printf( "INFO: Count Unique Import Libraries = %i\r\n", result.COUNT_UNIQUE_IMPORT_LIBRARIES );
		printf( "INFO: Generic Hash = 0x%llX\r\n", result.HASH_GENERIC );
		printf( "INFO: Specific Hash = 0x%llX\r\n", result.HASH_SPECIFIC );
	}

	return result;
}

unsigned __int64 pe_header::get_hash()
{
	//return rand() + (rand() << 32);
	if( _unique_hash == 0 )
		process_hash();
	return _unique_hash;
}

bool pe_header::build_pe_header( __int64 size, bool amd64 )
{
	return build_pe_header( size, amd64, 99 ); // Build it with as many sections as we can.
}

bool pe_header::build_pe_header( __int64 size, bool amd64, int num_sections_limit )
{
	if( _stream != NULL )
	{
		_raw_header_size = 0x2000;
		_raw_header = new unsigned char[_raw_header_size];
		memset( _raw_header, 0, _raw_header_size );
		_original_base = (void*) ((__int64) _original_base -  (__int64) _raw_header_size);
		_stream->update_base(-(__int64) _raw_header_size);

		// Build the old dos header
		_header_dos = (IMAGE_DOS_HEADER*) _raw_header;
		_header_dos->e_magic=0x5a4d;
		_header_dos->e_cblp=0x0090;
		_header_dos->e_cp=0x0003;
		_header_dos->e_crlc=0x0000;
		_header_dos->e_cparhdr=0x0004;
		_header_dos->e_minalloc=0x0000;
		_header_dos->e_maxalloc=0xffff;
		_header_dos->e_ss=0x0000;
		_header_dos->e_sp=0x00b8;
		_header_dos->e_csum=0x0000;
		_header_dos->e_ip=0x0000;
		_header_dos->e_cs=0x0000;
		_header_dos->e_lfarlc=0x0040;
		_header_dos->e_ovno=0x0000;
		memset( &_header_dos->e_res, 0, sizeof(WORD)*4 );
		_header_dos->e_oemid=0x0000;
		_header_dos->e_oeminfo=0x0000;
		memset( &_header_dos->e_res2, 0, sizeof(WORD)*10 );
		_header_dos->e_lfanew=0x000000e0;

		this->_parsed_dos = true;

		unsigned char* base_pe = _header_dos->e_lfanew + _raw_header;
		
		if( !amd64 )
		{
			// Build intel 32 bit PE header
			_header_pe32 = (IMAGE_NT_HEADERS32*) base_pe;
			_header_pe32->Signature = 0x00004550;
			_header_pe32->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
			_header_pe32->FileHeader.NumberOfSections = 1;
			_header_pe32->FileHeader.NumberOfSymbols = 0;
			_header_pe32->FileHeader.PointerToSymbolTable = 0;
			_header_pe32->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
			if( _options->ReconstructHeaderAsDll )
				_header_pe32->FileHeader.Characteristics = 0x0002; // Exe: 0x0002
			else
				_header_pe32->FileHeader.Characteristics = 0x2000; // Dll: 0x2000
			_header_pe32->OptionalHeader.Magic=0x10b;
			_header_pe32->OptionalHeader.MajorLinkerVersion=0x08;
			_header_pe32->OptionalHeader.MinorLinkerVersion=0x00;
			_header_pe32->OptionalHeader.SizeOfCode=0x00000000;
			_header_pe32->OptionalHeader.SizeOfInitializedData=0x00000000;
			_header_pe32->OptionalHeader.SizeOfUninitializedData=0x00000000;
			_header_pe32->OptionalHeader.AddressOfEntryPoint=0x2000; // Made up, start of first section
			_header_pe32->OptionalHeader.BaseOfCode=0x00002000;
			_header_pe32->OptionalHeader.ImageBase= (DWORD)_original_base; // Set to current address
			_header_pe32->OptionalHeader.SectionAlignment=0x00001000;
			_header_pe32->OptionalHeader.FileAlignment=0x000001000;
			_header_pe32->OptionalHeader.MajorOperatingSystemVersion=0x0004;
			_header_pe32->OptionalHeader.MinorOperatingSystemVersion=0x0000;
			_header_pe32->OptionalHeader.MajorImageVersion=0x0000;
			_header_pe32->OptionalHeader.MinorImageVersion=0x0000;
			_header_pe32->OptionalHeader.MajorSubsystemVersion=0x0005;
			_header_pe32->OptionalHeader.MinorSubsystemVersion=0x0002;
			_header_pe32->OptionalHeader.Win32VersionValue=0x00000000;
			_header_pe32->OptionalHeader.SizeOfImage=0x00006000;
			_header_pe32->OptionalHeader.SizeOfHeaders=0x00002000;
			_header_pe32->OptionalHeader.CheckSum=0x00000000;
			_header_pe32->OptionalHeader.Subsystem=0x0003;
			_header_pe32->OptionalHeader.DllCharacteristics=0x0000; // 0x2000
			_header_pe32->OptionalHeader.SizeOfStackReserve=0x0000000000100000;
			_header_pe32->OptionalHeader.SizeOfStackCommit=0x0000000000001000;
			_header_pe32->OptionalHeader.SizeOfHeapReserve=0x0000000000100000;
			_header_pe32->OptionalHeader.SizeOfHeapCommit=0x0000000000001000;
			_header_pe32->OptionalHeader.LoaderFlags=0x00000000;
			_header_pe32->OptionalHeader.NumberOfRvaAndSizes=0x00000010;
			memset( &_header_pe32->OptionalHeader.DataDirectory, 0, sizeof(IMAGE_DATA_DIRECTORY)*IMAGE_NUMBEROF_DIRECTORY_ENTRIES );

			_header_sections = (IMAGE_SECTION_HEADER*) (base_pe + sizeof(IMAGE_NT_HEADERS32));

			this->_parsed_pe_32 = true;
		}
		else
		{
			// Build intel 64 bit PE header
			_header_pe64 = (IMAGE_NT_HEADERS64*) base_pe;
			_header_pe64->Signature = 0x00004550;
			_header_pe64->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
			_header_pe64->FileHeader.NumberOfSections = 1;
			_header_pe64->FileHeader.NumberOfSymbols = 0;
			_header_pe64->FileHeader.PointerToSymbolTable = 0;
			_header_pe64->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
			if( _options->ReconstructHeaderAsDll )
				_header_pe64->FileHeader.Characteristics = 0x0002; // Exe: 0x0002
			else
				_header_pe64->FileHeader.Characteristics = 0x2000; // Dll: 0x2000
			_header_pe64->OptionalHeader.Magic=0x020b;
			_header_pe64->OptionalHeader.MajorLinkerVersion=0x08;
			_header_pe64->OptionalHeader.MinorLinkerVersion=0x00;
			_header_pe64->OptionalHeader.SizeOfCode=0x00000000;
			_header_pe64->OptionalHeader.SizeOfInitializedData=0x00000000;
			_header_pe64->OptionalHeader.SizeOfUninitializedData=0x00000000;
			
			// Select the entry point
			_header_pe64->OptionalHeader.AddressOfEntryPoint=0x2000; // Made up, start of first section
			_header_pe64->OptionalHeader.BaseOfCode=0x00002000;
			_header_pe64->OptionalHeader.ImageBase= (__int64)_original_base; // Set to current address
			_header_pe64->OptionalHeader.SectionAlignment=0x00001000;
			_header_pe64->OptionalHeader.FileAlignment=0x000001000;
			_header_pe64->OptionalHeader.MajorOperatingSystemVersion=0x0004;
			_header_pe64->OptionalHeader.MinorOperatingSystemVersion=0x0000;
			_header_pe64->OptionalHeader.MajorImageVersion=0x0000;
			_header_pe64->OptionalHeader.MinorImageVersion=0x0000;
			_header_pe64->OptionalHeader.MajorSubsystemVersion=0x0005;
			_header_pe64->OptionalHeader.MinorSubsystemVersion=0x0002;
			_header_pe64->OptionalHeader.Win32VersionValue=0x00000000;
			_header_pe64->OptionalHeader.SizeOfImage=0x00006000;
			_header_pe64->OptionalHeader.SizeOfHeaders=0x00002000;
			_header_pe64->OptionalHeader.CheckSum=0x00000000;
			_header_pe64->OptionalHeader.Subsystem=0x0003;
			_header_pe64->OptionalHeader.DllCharacteristics=0x0000;
			_header_pe64->OptionalHeader.SizeOfStackReserve=0x0000000000100000;
			_header_pe64->OptionalHeader.SizeOfStackCommit=0x0000000000001000;
			_header_pe64->OptionalHeader.SizeOfHeapReserve=0x0000000000100000;
			_header_pe64->OptionalHeader.SizeOfHeapCommit=0x0000000000001000;
			_header_pe64->OptionalHeader.LoaderFlags=0x00000000;
			_header_pe64->OptionalHeader.NumberOfRvaAndSizes=0x00000010;
			memset( &_header_pe64->OptionalHeader.DataDirectory, 0, sizeof(IMAGE_DATA_DIRECTORY)*IMAGE_NUMBEROF_DIRECTORY_ENTRIES );

			_header_sections = (IMAGE_SECTION_HEADER*) (base_pe + sizeof(IMAGE_NT_HEADERS64));

			this->_parsed_pe_64 = true;
		}
		
		// Create the sections
		_num_sections = 0;
		__int64 image_size = _raw_header_size;
		while( _stream->estimate_section_size(image_size) != 0 && image_size >= size && _num_sections < 99 && _num_sections < num_sections_limit )
		{
			__int64 est_size = _stream->estimate_section_size(image_size);
			
			_header_sections[_num_sections].PointerToRawData = image_size;
			_header_sections[_num_sections].SizeOfRawData = est_size;
			_header_sections[_num_sections].VirtualAddress = image_size;
			_header_sections[_num_sections].Misc.PhysicalAddress = image_size;
			_header_sections[_num_sections].Misc.VirtualSize = est_size;
			_header_sections[_num_sections].Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE; //_stream->get_region_characteristics(offset);
			char name[9];
			sprintf_s( name, 9, "pd_rec%i", _num_sections);
			memcpy( &_header_sections[_num_sections].Name, name, 8 );
			_header_sections[_num_sections].NumberOfLinenumbers = 0;
			_header_sections[_num_sections].NumberOfRelocations = 0;
			_header_sections[_num_sections].PointerToLinenumbers = 0;
			
			if( _options->Verbose )
				printf("%s: size %x\r\n", name, image_size);

			_num_sections++;
			image_size += est_size;
		}

		// Update the number of sections and image size
		if( !amd64 )
		{
			_header_pe32->FileHeader.NumberOfSections = _num_sections;
			_header_pe32->OptionalHeader.SizeOfImage = image_size;
		}
		else
		{
			_header_pe64->FileHeader.NumberOfSections = _num_sections;
			_header_pe64->OptionalHeader.SizeOfImage = image_size;
		}

		return true;
	}
	return false;
}

bool pe_header::process_pe_header( )
{
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loading PE header for %s.\r\n", this->get_name() );

	if( _stream != NULL )
	{
		// Request the block size of the first region
		_raw_header_size = _stream->block_size(0);
		_raw_header = new unsigned char[_raw_header_size];
		
		if( _raw_header_size >= 0x500 )
		{
			// Read in the PE header
			if( _stream->read(0, _raw_header_size, _raw_header, &_raw_header_size) && _raw_header_size >= 0x500 )
			{
				// Parse the PE header
				if( _raw_header_size > sizeof(IMAGE_DOS_HEADER) )
				{
					this->_header_dos = (IMAGE_DOS_HEADER*) _raw_header;
					
					if( _header_dos->e_magic == 0x5A4D )
					{
						// Successfully parsed dos header
						this->_parsed_dos = true;
						
						// Parse the PE header
						unsigned char* base_pe = _header_dos->e_lfanew + _raw_header;
						
						if( _test_read( _raw_header, _raw_header_size, base_pe, sizeof(IMAGE_NT_HEADERS64) ) )
						{
							// We are unsure if we need to process this as a 32bit or 64bit PE header, lets figure it out.
							// The first part is independent of the 32 or 64 bit definition.
							if( ((IMAGE_NT_HEADERS64*) base_pe)->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 )
							{
								// 32bit module
								this->_header_pe32 = ((IMAGE_NT_HEADERS32*) base_pe);

								if( _header_pe32->Signature == 0x4550 && _header_pe32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
								{
									this->_parsed_pe_32 = true;
									if( _options->Verbose )
										fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\r\n", this->get_name(), this->somewhat_parsed() );
									return true;
								}
							}
							else if( ((IMAGE_NT_HEADERS64*) base_pe)->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 ||
								((IMAGE_NT_HEADERS64*) base_pe)->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
							{
								// 64bit module
								this->_header_pe64 = ((IMAGE_NT_HEADERS64*) base_pe);

								if( _header_pe64->Signature == 0x4550 && _header_pe64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
								{
									this->_parsed_pe_64 = true;
									if( _options->Verbose )
										fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\r\n", this->get_name(), this->somewhat_parsed() );
									return true;
								}
							}
							else
							{
								// error
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if( _options->Verbose )
			fprintf( stderr, "INFO: Invalid stream.\r\n" );
	}
	
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\r\n", this->get_name(), this->somewhat_parsed() );

	return false;
}

bool pe_header::process_sections( )
{
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loading sections for %s.\r\n", this->get_name() );

	if( this->_parsed_pe_32 )
	{
		// Attempt to parse the sections
		unsigned char* base_pe = _header_dos->e_lfanew + _raw_header;
		unsigned char* base_sections = base_pe + sizeof(*_header_pe32);
		if( _header_pe32->FileHeader.NumberOfSections > 0x100 )
		{
			char* location = new char[FILEPATH_SIZE + 1];
			_stream->get_location(location, FILEPATH_SIZE + 1);
			fprintf( stderr, "WARNING: module '%s' at %s. Extremely large number of sections of 0x%x changed to 0x100 as part of sanity check.\r\n",
				this->get_name(), location, _header_pe32->FileHeader.NumberOfSections );
			_header_pe32->FileHeader.NumberOfSections = 0x100;
			delete[] location;
		}
		
		if( _test_read( _raw_header, _raw_header_size, base_sections, sizeof(IMAGE_SECTION_HEADER) ) )
		{
			// Has room for at least 1 section.

			if( !_test_read( _raw_header, _raw_header_size, base_sections, _header_pe32->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) ) )
			{
				// Parse the maximum number of sections possible
				char* location = new char[FILEPATH_SIZE + 1];
				_stream->get_location(location, FILEPATH_SIZE + 1);
				fprintf( stderr, "WARNING: module '%s' at %s. Number of sections being changed from 0x%x to 0x%x such that it will fit within the PE header buffer.\r\n",
					this->get_name(), location,
					_header_pe32->FileHeader.NumberOfSections,
					( (_raw_header + _raw_header_size - base_sections - 1) / sizeof(IMAGE_SECTION_HEADER) )
					);
				delete[] location;
				_header_pe32->FileHeader.NumberOfSections = ( (_raw_header + _raw_header_size - base_sections - 1) / sizeof(IMAGE_SECTION_HEADER) );
			}

			this->_parsed_sections = true;
			this->_num_sections = _header_pe32->FileHeader.NumberOfSections;
			this->_header_sections = (IMAGE_SECTION_HEADER*) base_sections;

			if( _options->Verbose )
			{
				for( int i = 0; i < this->_num_sections; i++ )
				{
					if( _test_read( _raw_header, _raw_header_size, this->_header_sections[i].Name, 0x40 ) )
						fprintf( stdout, "INFO: %s\t#%i\t%s\t0x%x\t0x%x\r\n", this->get_name(), i, this->_header_sections[i].Name, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
					else
						fprintf( stdout, "INFO: %s\t#%i\tINVALID ADDRESS\t0x%x\t0x%x\r\n", this->get_name(), i, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
				}
			}


			// Calculate the total size of the virtual image by inspecting the last section
			DWORD image_size = 0;
			if( this->_num_sections > 0 )
			{
				if( _header_sections[_num_sections - 1].Misc.VirtualSize > MAX_SECTION_SIZE )
				{
					// Smartly choose _header_pe32->OptionalHeader.SizeOfImage or last section plus max section size
					if( _header_pe32->OptionalHeader.SizeOfImage > _header_sections[_num_sections - 1].VirtualAddress &&
						  _header_pe32->OptionalHeader.SizeOfImage < _header_sections[_num_sections - 1].VirtualAddress + MAX_SECTION_SIZE )
					{
						// Use the _header_pe32->OptionalHeader.SizeOfImage, since it seems valid
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Image size of last section appears incorrect, using image size specified by optional header instead since it appears valid. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
								this->get_name(), location);
						delete[] location;
						image_size = _header_pe32->OptionalHeader.SizeOfImage;
					}
					else
					{
						// Assume a really large last section since _header_pe32->OptionalHeader.SizeOfImage appears invalid. 
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Image size of last section appears incorrect, using built-in max section size of 0x%x instead. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location,
							MAX_SECTION_SIZE * (_num_sections+1));
						delete[] location;
						image_size = _header_sections[_num_sections - 1].VirtualAddress + MAX_SECTION_SIZE;
					}
				}
				else
					image_size = _header_sections[_num_sections - 1].VirtualAddress +
											 _header_sections[_num_sections - 1].Misc.VirtualSize;
			}
			if( _header_pe32->OptionalHeader.SizeOfImage > image_size )
				image_size = _header_pe32->OptionalHeader.SizeOfImage;
			
			// Perform a sanity check on the resulting image size
			if( image_size > MAX_SECTION_SIZE * (_num_sections+1)  )
			{
				char* location = new char[FILEPATH_SIZE + 1];
				_stream->get_location(location, FILEPATH_SIZE + 1);
				fprintf( stderr, "WARNING: module '%s' at %s. Large image size of 0x%x changed to 0x%x as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
					this->get_name(), location,
					image_size, MAX_SECTION_SIZE * (_num_sections+1) );
				delete[] location;
				image_size = MAX_SECTION_SIZE * (_num_sections+1);
			}

			// Now lets build a proper image of this file with virtual alignment
			_image_size = image_size;
			_image = new unsigned char[_image_size];
			memset(_image, 0, _image_size);

			// Read in this full image
			if( _stream->file_alignment )
			{
				// Read in the full image from disk alignment
				
				// Read in the header
				SIZE_T num_read = 0;
				
				if( _test_read( _image, _image_size, _image, _header_pe32->OptionalHeader.SizeOfHeaders ) )
				{
					if( !_stream->read(0, _header_pe32->OptionalHeader.SizeOfHeaders, _image, &num_read ) && _options->Verbose )
					{
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n",this->get_name(), location, _header_pe32->OptionalHeader.SizeOfHeaders, num_read);
						delete[] location;
					}
				}
				else
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header.", this->get_name(), location);
					delete[] location;
				}



				// Loop through reading the sections into their respective virtual sections
				if( this->_parsed_sections )
				{
					for( int i = 0; i < this->_num_sections; i++ )
					{
						// Test the destination is valid
						if( _test_read( _image, _image_size,
							_image + (SIZE_T) this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData ) )
						{
							// Read in this section
							if( !_stream->read( this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData,
								_image + (SIZE_T) this->_header_sections[i].VirtualAddress, &num_read ) && _options->Verbose )
							{
								char* location = new char[FILEPATH_SIZE + 1];
								_stream->get_location(location, FILEPATH_SIZE + 1);
								fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in section %i of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n", this->get_name(), location, i, this->_header_sections[i].SizeOfRawData, num_read);
								delete[] location;
							}
						}
					}
				}
			}
			else
			{
				// Read in the full image from virtual alignment
				SIZE_T num_read = 0;
				if( !_stream->read( 0, _image_size, _image, &num_read ) && _options->Verbose )
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in image at 0x%llX of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n",this->get_name(), location, this->_stream->get_address(), _image_size, num_read);
					delete[] location;
				}
			}

			if( _options->Verbose )
				fprintf( stdout, "INFO: Loaded sections for %s with result: %d. %i sections found.\r\n", this->get_name(), this->_parsed_sections, ( this->_parsed_sections ? this->_num_sections : 0 )  );

			return true;
		}
	}
	else if( this->_parsed_pe_64 )
	{
		// Attempt to parse the sections
		unsigned char* base_pe = _header_dos->e_lfanew + _raw_header;
		unsigned char* base_sections = base_pe + sizeof(*_header_pe64);
		if( _header_pe64->FileHeader.NumberOfSections > 0x100 )
		{
			char* location = new char[FILEPATH_SIZE + 1];
			_stream->get_location(location, FILEPATH_SIZE + 1);
			fprintf( stderr, "WARNING: module '%s' at %s. Extremely large number of sections of 0x%x changed to 0x100 as part of sanity check.\r\n",
				this->get_name(), location, _header_pe64->FileHeader.NumberOfSections );
			_header_pe64->FileHeader.NumberOfSections = 0x100;
			delete[] location;
		}
		
		if( _test_read( _raw_header, _raw_header_size, base_sections, sizeof(IMAGE_SECTION_HEADER) ) )
		{
			// Has room for at least 1 section.

			if( !_test_read( _raw_header, _raw_header_size, base_sections, _header_pe64->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) ) )
			{
				// Parse the maximum number of sections possible
				char* location = new char[FILEPATH_SIZE + 1];
				_stream->get_location(location, FILEPATH_SIZE + 1);
				fprintf( stderr, "WARNING: module '%s' at %s. Number of sections being changed from 0x%x to 0x%x such that it will fit within the PE header buffer.\r\n",
					this->get_name(), location,
					_header_pe64->FileHeader.NumberOfSections,
					( (_raw_header + _raw_header_size - base_sections - 1) / sizeof(IMAGE_SECTION_HEADER) )
					);
				delete[] location;
				_header_pe64->FileHeader.NumberOfSections = ( (_raw_header + _raw_header_size - base_sections - 1) / sizeof(IMAGE_SECTION_HEADER) );
			}

			this->_parsed_sections = true;
			this->_num_sections = _header_pe64->FileHeader.NumberOfSections;
			this->_header_sections = (IMAGE_SECTION_HEADER*) base_sections;
	
			if( _options->Verbose )
			{
				for( int i = 0; i < this->_num_sections; i++ )
				{
					fprintf( stdout, "INFO: %s\t#%i\t%s\t0x%x\t0x%x\r\n", this->get_name(), i, this->_header_sections[i].Name, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
				}
			}

			// Calculate the total size of the virtual image by inspecting the last section
			DWORD image_size = 0;
			if( this->_num_sections > 0 )
			{
				if( _header_sections[_num_sections - 1].Misc.VirtualSize > MAX_SECTION_SIZE )
				{
					// Smartly choose _header_pe64->OptionalHeader.SizeOfImage or last section plus max section size
					if( _header_pe64->OptionalHeader.SizeOfImage > _header_sections[_num_sections - 1].VirtualAddress &&
						  _header_pe64->OptionalHeader.SizeOfImage < _header_sections[_num_sections - 1].VirtualAddress + MAX_SECTION_SIZE )
					{
						// Use the _header_pe64->OptionalHeader.SizeOfImage, since it seems valid
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Image size of last section appears incorrect, using image size specified by optional header instead since it appears valid. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
								this->get_name(), location);
						delete[] location;
						image_size = _header_pe64->OptionalHeader.SizeOfImage;
					}
					else
					{
						// Assume a really large last section since _header_pe64->OptionalHeader.SizeOfImage appears invalid. 
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Image size of last section appears incorrect, using built-in max section size of 0x%x instead. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location,
							MAX_SECTION_SIZE * (_num_sections+1));
						delete[] location;
						image_size = _header_sections[_num_sections - 1].VirtualAddress + MAX_SECTION_SIZE;
					}
				}
				else
					image_size = _header_sections[_num_sections - 1].VirtualAddress +
											 _header_sections[_num_sections - 1].Misc.VirtualSize;
			}
			if( _header_pe64->OptionalHeader.SizeOfImage > image_size )
				image_size = _header_pe64->OptionalHeader.SizeOfImage;
			
			// Perform a sanity check on the resulting image size
			if( image_size > MAX_SECTION_SIZE * (_num_sections+1)  )
			{
				char* location = new char[FILEPATH_SIZE + 1];
				_stream->get_location(location, FILEPATH_SIZE + 1);
				fprintf( stderr, "WARNING: module '%s' at %s. Large image size of 0x%x changed to 0x%x as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
					this->get_name(), location,
					image_size, MAX_SECTION_SIZE * (_num_sections+1) );
				delete[] location;
				image_size = MAX_SECTION_SIZE * (_num_sections+1);
			}

			// Now lets build a proper image of this file with virtual alignment
			_image_size = image_size;
			_image = new unsigned char[_image_size];
			memset(_image, 0, _image_size);

			// Read in this full image
			if( _stream->file_alignment )
			{
				// Read in the full image from disk alignment
				
				// Read in the header
				SIZE_T num_read = 0;
				
				if( _test_read( _image, _image_size, _image, _header_pe64->OptionalHeader.SizeOfHeaders ) )
				{
					if( !_stream->read(0, _header_pe64->OptionalHeader.SizeOfHeaders, _image, &num_read ) && _options->Verbose )
					{
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n",this->get_name(), location, _header_pe64->OptionalHeader.SizeOfHeaders, num_read);
						delete[] location;
					}
				}
				else
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header.", this->get_name(), location);
					delete[] location;
				}



				// Loop through reading the sections into their respective virtual sections
				if( this->_parsed_sections )
				{
					for( int i = 0; i < this->_num_sections; i++ )
					{
						// Test the destination is valid
						if( _test_read( _image, _image_size,
							_image + (SIZE_T) this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData ) )
						{
							// Read in this section
							if( !_stream->read( this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData,
								_image + (SIZE_T) this->_header_sections[i].VirtualAddress, &num_read ) && _options->Verbose )
							{
								char* location = new char[FILEPATH_SIZE + 1];
								_stream->get_location(location, FILEPATH_SIZE + 1);
								fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in section %i of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n", this->get_name(), location, i, this->_header_sections[i].SizeOfRawData, num_read);
								delete[] location;
							}
						}
					}
				}
			}
			else
			{
				// Read in the full image from virtual alignment
				SIZE_T num_read = 0;
				if( !_stream->read( 0, _image_size, _image, &num_read ) && _options->Verbose )
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in image at 0x%llX of size 0x%x. Was only able to read 0x%x bytes from this region.\r\n",this->get_name(), location, this->_stream->get_address(), _image_size, num_read);
					delete[] location;
				}
			}

			if( _options->Verbose )
				fprintf( stdout, "INFO: Loaded sections for %s with result: %d. %i sections found.\r\n", this->get_name(), this->_parsed_sections, ( this->_parsed_sections ? this->_num_sections : 0 )  );

			return true;
		}
	}

	if( _options->Verbose )
		fprintf( stdout, "INFO: Failed to load sections for %s.\r\n", this->get_name() );

	return false;
}

bool pe_header::process_disk_image( export_list* exports )
{
	if( this->_parsed_sections )
	{
		if( this->_parsed_pe_32 )
		{
			// Reconstruct PE imports aggressively using our knowledge of all the exports addresses in this process
			// Technique:
			//   1. 'exports' defines all valid export addresses in this process
			//   2. Find any binary that points to a valid export in this rpocess
			//   3. Add a new section for the new HintName Array and Import Address Table
			//   4. For each binary patch found above, add a HintName and ImportAddress point so the loads
			//      will recognize it correctly for analysis (IDA).
			unsigned char* larger_image;
			__int64 larger_image_size;
			if( _options->ImportRec )
			{
				// Start the with the original import descriptor list
				pe_imports* peimp = new pe_imports( _image, _image_size, _header_import_descriptors, false );
				
				// Add matches to exports in this process
				int count = 0;
				unsigned __int64 cand_last = 0;
				for(__int64 offset = 0; offset < _image_size - 8; offset+=4 )
				{
					// Check if this 4-gram or 8-gram points to an export
					unsigned __int64 cand = *((__int32*)(_image + offset));

					if ( cand_last != cand && exports->contains( cand ) )
					{
						export_entry entry = exports->find(cand);

						// Add this to be reconstructed as an import
						if (entry.name != NULL)
							peimp->add_fixup(entry.library_name, entry.name, offset, this->_parsed_pe_64);
						else
							peimp->add_fixup(entry.library_name, entry.ord, offset, this->_parsed_pe_64);
						count++;
					}
					else
					{
						cand_last = cand;
					}
				}
				if( _options->Verbose )
					printf( "INFO: Reconstructing %i imports.\r\n", count );
				
				// Increase the image size for a new section
				__int64 descriptor_size = 0;
				__int64 data_size = 0;
				peimp->get_table_size( descriptor_size, data_size );
				__int64 new_section_size = this->_section_align(data_size+descriptor_size, this->_header_pe32->OptionalHeader.SectionAlignment);
				
				
				// Increase the size of the last section
				_header_sections[_num_sections-1].Misc.VirtualSize = this->_section_align(_header_sections[_num_sections-1].Misc.VirtualSize, this->_header_pe32->OptionalHeader.SectionAlignment) + new_section_size;
				_header_sections[_num_sections-1].SizeOfRawData = _header_sections[_num_sections-1].Misc.VirtualSize;

				larger_image_size = this->_section_align((long long) this->_image_size, this->_header_pe32->OptionalHeader.SectionAlignment) + new_section_size;
				larger_image = new unsigned char[larger_image_size];
				memset(larger_image, 0, larger_image_size);
				memcpy(larger_image, _image, _image_size);

				if( _options->Verbose )
					printf( "INFO: Writing added import table.\r\n" );
				
				// Write to the new section
				peimp->build_table( larger_image + this->_section_align((long long) _image_size, this->_header_pe32->OptionalHeader.SectionAlignment), new_section_size, (__int64) _image_size, (__int64) 0, descriptor_size );
				
				if( _options->Verbose )
					printf( "INFO: Updating import data directory.\r\n" );

				// Update the PE header to refer to it
				_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = this->_section_align((long long) _image_size, this->_header_pe32->OptionalHeader.SectionAlignment);
				_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptor_size;
				
				delete peimp;
			}
			else
			{
				larger_image_size = _image_size;
				larger_image = new unsigned char[larger_image_size];
				memset(larger_image, 0, larger_image_size);
				memcpy(larger_image, _image, _image_size);
			}
			
			if( _original_base != 0 )
			{
				// Adjust the preferred image base, this way the relocations doesn't have to be fixed
				_header_pe32->OptionalHeader.ImageBase = (DWORD) _original_base;
			}

			// Change the physical alignment to use the virtual alignment
			if( _options->Verbose )
					printf( "INFO: Adjusting file alignment to %x.\r\n", _header_pe32->OptionalHeader.SectionAlignment);
			_header_pe32->OptionalHeader.FileAlignment = _header_pe32->OptionalHeader.SectionAlignment;
			
			// Adjust the physical size of each section to use the virtual size
			_header_pe32->OptionalHeader.SizeOfHeaders = _section_align(_header_pe32->OptionalHeader.SizeOfHeaders, _header_pe32->OptionalHeader.FileAlignment);
			DWORD required_space = _section_align( _header_pe32->OptionalHeader.SizeOfHeaders, _header_pe32->OptionalHeader.SectionAlignment);
			
			for( int i = 0; i < _num_sections; i++ )
			{
				// Correct the VirtualSize of the section if it is too large
				if( this->_header_sections[i].Misc.VirtualSize > MAX_SECTION_SIZE  )
				{
					if( _options->Verbose )
						printf( "INFO: Calculating required space for section %i.\r\n", i);

					if( i + 1 < _num_sections &&
						this->_header_sections[i+1].VirtualAddress > this->_header_sections[i].VirtualAddress &&
						this->_header_sections[i+1].VirtualAddress < this->_header_sections[i].VirtualAddress + MAX_SECTION_SIZE )
					{
						// Calculate the virtual size manually
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x changed to 0x%x based on image size as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, this->_header_sections[i+1].VirtualAddress - this->_header_sections[i].VirtualAddress );
						delete[] location;
						this->_header_sections[i].Misc.VirtualSize = this->_header_sections[i+1].VirtualAddress - this->_header_sections[i].VirtualAddress;
					}
					else
					{
						// Use MAX_SECTION_SIZE
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x changed to 0x%x based on maximum section size as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, MAX_SECTION_SIZE );
						delete[] location;
						this->_header_sections[i].Misc.VirtualSize = MAX_SECTION_SIZE;
					}
				}
				
				// Truncate VirtualSize to fit inside image size
				if( this->_header_sections[i].Misc.VirtualSize + this->_header_sections[i].VirtualAddress > larger_image_size )
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					DWORD new_size = larger_image_size - this->_header_sections[i].VirtualAddress;
					fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x being truncated to 0x%x to fit within the image size. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
						this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, new_size );
					delete[] location;
					this->_header_sections[i].Misc.VirtualSize = new_size;
				}
				
				// Adjust the physical size to be at least the same size as the virtual size
				if( this->_header_sections[i].Misc.VirtualSize > _header_sections[i].SizeOfRawData )
				{
					_header_sections[i].SizeOfRawData = this->_header_sections[i].Misc.VirtualSize;
				}
				
				// Update the pointer to raw data to be correct
				_header_sections[i].PointerToRawData = required_space;
				
				required_space = _section_align( required_space + _header_sections[i].SizeOfRawData, _header_pe32->OptionalHeader.FileAlignment );
			}
			
			// Set the size of image
			_header_pe32->OptionalHeader.SizeOfImage = required_space;
			
			if( _options->Verbose )
						printf( "INFO: Copying the corrected memory PE header into file PE header format.\r\n");

			// Copy over the modified PE header into the imaged version
			if( _test_read( larger_image, larger_image_size, larger_image, _header_pe32->OptionalHeader.SizeOfHeaders ) &&
				_test_read( _raw_header, _raw_header_size, _raw_header, _header_pe32->OptionalHeader.SizeOfHeaders ) )
			{
				memcpy( larger_image, _raw_header, _header_pe32->OptionalHeader.SizeOfHeaders );
			}
			else if( _test_read( larger_image, larger_image_size, larger_image, _raw_header_size ) &&
					 _test_read( _raw_header, _raw_header_size, _raw_header, _raw_header_size ) )
			{
				memcpy( larger_image, _raw_header, _raw_header_size );
			}
			
			if( _header_pe32->OptionalHeader.SectionAlignment >= _header_pe32->OptionalHeader.FileAlignment )
			{
				// Pack it down into a disk image of the file
				if( _options->Verbose )
						printf( "INFO: Packing down memory sections into the file.\r\n");

				// Allocate the necessary space for the physical image and initialize it to zero
				_disk_image_size = required_space;
				_disk_image = new unsigned char[_disk_image_size];
				memset(_disk_image, 0, _disk_image_size);
				
				// Copy the header
				if( _test_read( _disk_image, _disk_image_size, _disk_image, _section_align(_header_pe32->OptionalHeader.SizeOfHeaders, _header_pe32->OptionalHeader.FileAlignment) ) &&
					_test_read( larger_image, larger_image_size, larger_image, _section_align(_header_pe32->OptionalHeader.SizeOfHeaders, _header_pe32->OptionalHeader.FileAlignment) ) )
				{
					memcpy( _disk_image, larger_image, _header_pe32->OptionalHeader.SizeOfHeaders );
				}
				
				if( _parsed_sections )
				{
					// Copy the sections one-by-one
					for( int i = 0; i < _num_sections; i++ )
					{
						if( _options->Verbose )
							printf( "INFO: Packing down section %i.\r\n", i);

						// Copy this section if the source and destination are both within acceptable bounds
						if( _test_read( _disk_image, _disk_image_size,
							_disk_image + (SIZE_T) _header_sections[i].PointerToRawData, _header_sections[i].SizeOfRawData ) &&
							_test_read( larger_image, larger_image_size,
							larger_image + (SIZE_T) _header_sections[i].VirtualAddress, _header_sections[i].SizeOfRawData ))
						{
							memcpy( _disk_image + _header_sections[i].PointerToRawData, larger_image + _header_sections[i].VirtualAddress, _header_sections[i].SizeOfRawData );
						}
					}
				}

				delete [] larger_image;
				if( _options->Verbose )
					printf( "INFO: Done processing disk image.\r\n");

				return true;
			}
			delete [] larger_image;	
		}
		else if( this->_parsed_pe_64 )
		{
			// Reconstruct PE imports aggressively using our knowledge of all the exports addresses in this process
			// Technique:
			//   1. 'exports' defines all valid export addresses in this process
			//   2. Find any binary that points to a valid export in this rpocess
			//   3. Add a new section for the new HintName Array and Import Address Table
			//   4. For each binary patch found above, add a HintName and ImportAddress point so the loads
			//      will recognize it correctly for analysis (IDA).
			unsigned char* larger_image;
			__int64 larger_image_size;
			if( _options->ImportRec )
			{
				// Start the with the original import descriptor list
				pe_imports* peimp = new pe_imports( _image, _image_size, _header_import_descriptors, true );
				
				// Add matches to exports in this process
				int count = 0;
				unsigned __int64 cand_last = 0;
				for(__int64 offset = 0; offset < _image_size - 8; offset+=4 )
				{
					// Check if this 4-gram or 8-gram points to an export
					unsigned __int64 cand = *((unsigned __int64*)(_image + offset));

					if (cand_last != cand && exports->contains(cand))
					{
						export_entry entry = exports->find(cand);

						// Add this to be reconstructed as an import
						if (entry.name != NULL)
							peimp->add_fixup(entry.library_name, entry.name, offset, this->_parsed_pe_64);
						else
							peimp->add_fixup(entry.library_name, entry.ord, offset, this->_parsed_pe_64);
						count++;
					}
					else
					{
						cand_last = cand;
					}
				}
				if( _options->Verbose )
					printf( "INFO: Reconstructing %i imports.\r\n", count );
				
				// Increase the image size for a new section
				__int64 descriptor_size = 0;
				__int64 data_size = 0;
				peimp->get_table_size( descriptor_size, data_size );
				__int64 new_section_size = this->_section_align(data_size+descriptor_size, this->_header_pe64->OptionalHeader.SectionAlignment);
				
				
				// Increase the size of the last section
				_header_sections[_num_sections-1].Misc.VirtualSize = this->_section_align(_header_sections[_num_sections-1].Misc.VirtualSize, this->_header_pe64->OptionalHeader.SectionAlignment) + new_section_size;
				_header_sections[_num_sections-1].SizeOfRawData = _header_sections[_num_sections-1].Misc.VirtualSize;

				larger_image_size = this->_section_align((long long) this->_image_size, this->_header_pe64->OptionalHeader.SectionAlignment) + new_section_size;
				
				larger_image = new unsigned char[larger_image_size];
				memset(larger_image, 0, larger_image_size);
				memcpy(larger_image, _image, _image_size);

				if( _options->Verbose )
					printf( "INFO: Writing added import table.\r\n" );
				
				// Write to the new section
				peimp->build_table( larger_image + this->_section_align((long long) this->_image_size, this->_header_pe64->OptionalHeader.SectionAlignment), new_section_size, (__int64) _image_size, (__int64) 0, descriptor_size );
				
				if( _options->Verbose )
					printf( "INFO: Updating import data directory.\r\n" );

				// Update the PE header to refer to it
				_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = this->_section_align((long long) this->_image_size, this->_header_pe64->OptionalHeader.SectionAlignment);
				_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptor_size;
				
				delete peimp;
			}
			else
			{
				larger_image_size = _image_size;
				larger_image = new unsigned char[larger_image_size];
				memset(larger_image, 0, larger_image_size);
				memcpy(larger_image, _image, _image_size);
			}
			
				

			if( _original_base != 0 )
			{
				// Adjust the preferred image base, this way the relocations doesn't have to be fixed
				_header_pe64->OptionalHeader.ImageBase = reinterpret_cast<__int64> (_original_base);
			}

			// Change the physical alignment to use the virtual alignment
			if( _options->Verbose )
					printf( "INFO: Adjusting file alignment to %x.\r\n", _header_pe64->OptionalHeader.SectionAlignment);
			_header_pe64->OptionalHeader.FileAlignment = _header_pe64->OptionalHeader.SectionAlignment;
			
			// Adjust the physical size of each section to use the virtual size
			_header_pe64->OptionalHeader.SizeOfHeaders = _section_align(_header_pe64->OptionalHeader.SizeOfHeaders, _header_pe64->OptionalHeader.FileAlignment);
			DWORD required_space = _section_align( _header_pe64->OptionalHeader.SizeOfHeaders, _header_pe64->OptionalHeader.SectionAlignment);
			
			for( int i = 0; i < _num_sections; i++ )
			{
				// Correct the VirtualSize of the section if it is too large
				if( this->_header_sections[i].Misc.VirtualSize > MAX_SECTION_SIZE  )
				{
					if( _options->Verbose )
						printf( "INFO: Calculating required space for section %i.\r\n", i);

					if( i + 1 < _num_sections &&
						this->_header_sections[i+1].VirtualAddress > this->_header_sections[i].VirtualAddress &&
						this->_header_sections[i+1].VirtualAddress < this->_header_sections[i].VirtualAddress + MAX_SECTION_SIZE )
					{
						// Calculate the virtual size manually
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x changed to 0x%x based on image virtual size as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, this->_header_sections[i+1].VirtualAddress - this->_header_sections[i].VirtualAddress );
						delete[] location;
						this->_header_sections[i].Misc.VirtualSize = this->_header_sections[i+1].VirtualAddress - this->_header_sections[i].VirtualAddress;
					}
					else
					{
						// Use MAX_SECTION_SIZE
						char* location = new char[FILEPATH_SIZE + 1];
						_stream->get_location(location, FILEPATH_SIZE + 1);
						fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x changed to 0x%x based on maximum section size as part of sanity check. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
							this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, MAX_SECTION_SIZE );
						delete[] location;
						this->_header_sections[i].Misc.VirtualSize = MAX_SECTION_SIZE;
					}
				}
				
				// Truncate VirtualSize to fit inside image size
				if( this->_header_sections[i].Misc.VirtualSize + this->_header_sections[i].VirtualAddress > larger_image_size )
				{
					char* location = new char[FILEPATH_SIZE + 1];
					_stream->get_location(location, FILEPATH_SIZE + 1);
					DWORD new_size = larger_image_size - this->_header_sections[i].VirtualAddress;
					fprintf( stderr, "WARNING: module '%s' at %s. Large section size for section %i of 0x%x being truncated to 0x%x to fit within the image size. This could be as a result of a custom code to load a library by means other than LoadLibrary().\r\n",
						this->get_name(), location, i, this->_header_sections[i].Misc.VirtualSize, new_size );
					delete[] location;
					this->_header_sections[i].Misc.VirtualSize = new_size;
				}
				
				// Adjust the physical size to be at least the same size as the virtual size
				if( this->_header_sections[i].Misc.VirtualSize > _header_sections[i].SizeOfRawData )
				{
					_header_sections[i].SizeOfRawData = this->_header_sections[i].Misc.VirtualSize;
				}
				
				// Update the pointer to raw data to be correct
				_header_sections[i].PointerToRawData = required_space;
				
				required_space = _section_align( required_space + _header_sections[i].SizeOfRawData, _header_pe64->OptionalHeader.FileAlignment );
			}
			
			// Set the size of image
			_header_pe64->OptionalHeader.SizeOfImage = required_space;
			
			if( _options->Verbose )
				printf( "INFO: Copying the corrected memory PE header into file PE header format.\r\n");

			// Copy over the modified PE header into the imaged version
			if( _test_read( larger_image, larger_image_size, larger_image, _header_pe64->OptionalHeader.SizeOfHeaders ) &&
				_test_read( _raw_header, _raw_header_size, _raw_header, _header_pe64->OptionalHeader.SizeOfHeaders ) )
			{
				memcpy( larger_image, _raw_header, _header_pe64->OptionalHeader.SizeOfHeaders );
			}
			else if( _test_read( larger_image, larger_image_size, larger_image, _raw_header_size ) &&
					 _test_read( _raw_header, _raw_header_size, _raw_header, _raw_header_size ) )
			{
				memcpy( larger_image, _raw_header, _raw_header_size );
			}
			
			if( _header_pe64->OptionalHeader.SectionAlignment >= _header_pe64->OptionalHeader.FileAlignment )
			{
				// Pack it down into a disk image of the file
				if( _options->Verbose )
						printf( "INFO: Packing down memory sections into the file.\r\n");

				// Allocate the necessary space for the physical image and initialize it to zero
				_disk_image_size = required_space;
				_disk_image = new unsigned char[_disk_image_size];
				memset(_disk_image, 0, _disk_image_size);
				
				// Copy the header
				if( _test_read( _disk_image, _disk_image_size, _disk_image, _section_align(_header_pe64->OptionalHeader.SizeOfHeaders, _header_pe64->OptionalHeader.FileAlignment) ) &&
					_test_read( larger_image, larger_image_size, larger_image, _section_align(_header_pe64->OptionalHeader.SizeOfHeaders, _header_pe64->OptionalHeader.FileAlignment) ) )
				{
					memcpy( _disk_image, larger_image, _header_pe64->OptionalHeader.SizeOfHeaders );
				}
				
				if( _parsed_sections )
				{
					// Copy the sections one-by-one
					for( int i = 0; i < _num_sections; i++ )
					{
						if( _options->Verbose )
							printf( "INFO: Packing down section %i.\r\n", i);

						// Copy this section if the source and destination are both within acceptable bounds
						if( _test_read( _disk_image, _disk_image_size,
							_disk_image + (SIZE_T) _header_sections[i].PointerToRawData, _header_sections[i].SizeOfRawData ) &&
							_test_read( larger_image, larger_image_size,
							larger_image + (SIZE_T) _header_sections[i].VirtualAddress, _header_sections[i].SizeOfRawData ))
						{
							memcpy( _disk_image + _header_sections[i].PointerToRawData, larger_image + _header_sections[i].VirtualAddress, _header_sections[i].SizeOfRawData );
						}
					}
				}

				delete [] larger_image;

				if( _options->Verbose )
					printf( "INFO: Done processing disk image.\r\n");

				return true;
			}
			delete [] larger_image;	
		}
	}
	return false;
}

bool pe_header::process_import_directory( )
{
	if( this->_parsed_pe_32 )
	{
		// Test case for import reconstruction - destroy it :)
		//_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0;
		//_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = 0;

		if( _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != 0 )
		{
			unsigned char* base_imports = _image + _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

			// Count the number of IMAGE_IMPORT_DESCRIPTORs
			_header_import_descriptors_count = 0;
			bool more;
			do
			{
				more = false;
				if( _test_read( _image,_image_size, base_imports + _header_import_descriptors_count * sizeof(IMAGE_IMPORT_DESCRIPTOR ),
					sizeof(IMAGE_IMPORT_DESCRIPTOR ) ) )
				{
					IMAGE_IMPORT_DESCRIPTOR* current = &((IMAGE_IMPORT_DESCRIPTOR*) base_imports)[_header_import_descriptors_count];
					if( current->Characteristics != 0 || current->FirstThunk != 0 || current->ForwarderChain != 0 || current->Name != 0 )
					{
						more = true;
						_header_import_descriptors_count++;
					}
				}
			}while(more);

			if( _options->Verbose )
				printf("Found %i import descriptors.\r\n", _header_import_descriptors_count);

			if( _header_import_descriptors_count > 0 )
			{
				// Load the IMAGE_IMPORT_DESCRIPTOR array
				_header_import_descriptors = (IMAGE_IMPORT_DESCRIPTOR*) base_imports;

				// Now process and fix the contents of each of these IMAGE_IMPORT_DESCRIPTORs. We assume the
				// OriginalFirstThunk HintName array is valid.
				// TODO: Upgrade this to assume OriginalFirstThunk array is invalid.
				for( int i = 0; i < _header_import_descriptors_count; i++ )
				{
					int num_iat_entries = 0;
					bool more;
					do{
						more = false;
						if( _test_read( _image, _image_size, _image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) ) &&
							_test_read( _image, _image_size, _image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) ) &&
							*((DWORD*)(_image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32))) != 0 &&
							*((DWORD*)(_image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32))) != 0)
						{
							memcpy( _image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32),
								_image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) );

							more = true;
							num_iat_entries++;
						}
						// TODO: Parse the import table information

					}while(more);
					
					if( _options->Verbose )
							printf("Reconstructed %i thunk data entries.\r\n", num_iat_entries);
				}
			}
		}
		return true;
	}
	else if( this->_parsed_pe_64 )
	{
		// Test case for import reconstruction - destroy it :)
		//_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0;
		//_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = 0;

		if( _header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != 0 )
		{
			unsigned char* base_imports = _image + _header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

			// Count the number of IMAGE_IMPORT_DESCRIPTORs
			_header_import_descriptors_count = 0;
			bool more;
			do
			{
				more = false;
				if( _test_read( _image,_image_size, base_imports + _header_import_descriptors_count * sizeof(IMAGE_IMPORT_DESCRIPTOR ),
					sizeof(IMAGE_IMPORT_DESCRIPTOR ) ) )
				{
					IMAGE_IMPORT_DESCRIPTOR* current = &((IMAGE_IMPORT_DESCRIPTOR*) base_imports)[_header_import_descriptors_count];
					if( current->Characteristics != 0 || current->FirstThunk != 0 || current->ForwarderChain != 0 || current->Name != 0 )
					{
						more = true;
						_header_import_descriptors_count++;
					}
				}
			}while(more);

			if( _options->Verbose )
				printf("Found %i import descriptors.\r\n", _header_import_descriptors_count);

			if( _header_import_descriptors_count > 0 )
			{
				// Load the IMAGE_IMPORT_DESCRIPTOR array
				_header_import_descriptors = (IMAGE_IMPORT_DESCRIPTOR*) base_imports;

				// Now process and fix the contents of each of these IMAGE_IMPORT_DESCRIPTORs. We assume the
				// OriginalFirstThunk HintName array is valid.
				// TODO: Upgrade this to assume OriginalFirstThunk array is invalid.
				for( int i = 0; i < _header_import_descriptors_count; i++ )
				{
					int num_iat_entries = 0;
					bool more;
					do{
						more = false;
						if( _test_read( _image, _image_size, _image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) ) &&
							_test_read( _image, _image_size, _image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) ) &&
							*((DWORD*)(_image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32))) != 0 &&
							*((DWORD*)(_image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32))) != 0)
						{
							memcpy( _image + _header_import_descriptors[i].FirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32),
								_image + _header_import_descriptors[i].OriginalFirstThunk + num_iat_entries*sizeof(_IMAGE_THUNK_DATA32), sizeof(_IMAGE_THUNK_DATA32) );

							more = true;
							num_iat_entries++;
						}
						// TODO: Parse the import table information

					}while(more);
					
					if( _options->Verbose )
							printf("Reconstructed %i thunk data entries.\r\n", num_iat_entries);
				}
			}
		}
		return true;
	}

	return false;
}

bool pe_header::process_export_directory( )
{
	_header_import_descriptors_count = 0;

	if( (this->_parsed_pe_32 || this->_parsed_pe_64) && _image != NULL )
	{
		unsigned char* base_exports;
		if( _parsed_pe_32 )
			base_exports = _image + _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		else
			base_exports = _image + _header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		
		if( _test_read( _image,_image_size, base_exports, sizeof(IMAGE_EXPORT_DIRECTORY ) ) )
		{
			_header_export_directory = ((IMAGE_EXPORT_DIRECTORY*) base_exports);
			
			// Parse this export directory
			_export_list = new export_list();
			_export_list->add_exports( _image, _image_size, (__int64) _original_base, _header_export_directory, this->_parsed_pe_64);
			
			return true;
		}
	}

	return false;
}

bool pe_header::_test_read( unsigned char* buffer, SIZE_T length, unsigned char* read_ptr, SIZE_T read_length )
{
	return read_ptr >= buffer && read_ptr + read_length <= buffer + length;
}

pe_header::~pe_header(void)
{
	if( this->_stream != NULL )
		delete this->_stream;
	if( this->_image_size != 0 )
		delete[] _image;
	if( this->_raw_header_size != 0 )
		delete[] _raw_header;
	if( this->_disk_image_size != 0 )
		delete[] _disk_image;
	if( this->_name_filepath_long != 0 )
		delete[] _name_filepath_long;
	if( this->_name_filepath_short != 0 )
		delete[] _name_filepath_short;
	if( this->_name_original_exports != 0 )
		delete[] _name_original_exports;
	if( this->_name_original_manifest != 0 )
		delete[] _name_original_manifest;
	if( this->_name_symbols_path != 0 )
		delete[] _name_symbols_path;
	if( this->_export_list != NULL )
		delete _export_list;
}


DWORD pe_header::_section_align( DWORD address, DWORD alignment)
{
	// Round the address up to the nearest section alignment
	if( alignment > 0 && address % alignment > 0 )
		return (address - (address % alignment)) + alignment;
	return address;
}

__int64 pe_header::_section_align( __int64 address, DWORD alignment)
{
	// Round the address up to the nearest section alignment
	if( address % alignment > 0 )
		return (address - (address % alignment)) + alignment;
	return address;
}