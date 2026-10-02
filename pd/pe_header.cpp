#include "StdAfx.h"
#include "pe_header.h"
#include "dump_output.h"
#include <limits.h>
#include <memory>

namespace
{
	bool image_size_fits(__int64 size)
	{
		return size >= 0 && static_cast<unsigned __int64>(size) <= MAX_PE_IMAGE_SIZE &&
			static_cast<unsigned __int64>(size) <= SIZE_MAX && size <= MAXDWORD;
	}
}

void pe_header::_clear_images()
{
	delete[] _image;
	delete[] _disk_image;
	delete _export_list;
	_image = _disk_image = NULL;
	_image_size = _disk_image_size = 0;
	_export_list = NULL;
	_header_export_directory = NULL;
	_header_import_descriptors = NULL;
	_header_import_descriptors_count = 0;
	_header_sections = NULL;
	_num_sections = 0;
	_parsed_sections = false;
	_unique_hash = _unique_hash_ep = _unique_hash_ep_short = 0;
}

void pe_header::_clear_header()
{
	_clear_images();
	delete[] _raw_header;
	_raw_header = NULL;
	_raw_header_size = 0;
	_header_dos = NULL;
	_header_pe32 = NULL;
	_header_pe64 = NULL;
	_parsed_dos = _parsed_pe_32 = _parsed_pe_64 = false;
}

bool pe_header::_reject_size()
{
	fprintf(stderr, "WARNING: module '%s': invalid PE range or image exceeding the 256 MiB limit rejected.\n", get_name());
	return false;
}

pe_header::pe_header( char* filename, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	this->_stream = (stream_wrapper*) new file_stream( filename );
	_original_base = 0;
	_unique_hash = 0;
	_unique_hash_ep = 0;
	_unique_hash_ep_short = 0;

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
		fprintf( stdout, "INFO: Initialized header for module name %s.\n", this->get_name() );
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
	_unique_hash_ep = 0;
	_unique_hash_ep_short = 0;

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
		fprintf( stdout, "INFO: Initialized header for module name %s.\n", this->get_name() );
}

pe_header::pe_header( DWORD pid, module_list* modules, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	_unique_hash = 0;
	_unique_hash_ep = 0;
	_unique_hash_ep_short = 0;

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
		fprintf( stdout, "INFO: Initialized header for module name %s.\n", this->get_name() );
}

pe_header::pe_header( HANDLE ph, void* base, module_list* modules, PD_OPTIONS* options )
{
	this->_options = options;
	this->_image_size = 0;
	this->_raw_header_size = 0;
	this->_disk_image_size = 0;
	_unique_hash = 0;
	_unique_hash_ep = 0;
	_unique_hash_ep_short = 0;

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

	this->_stream = (stream_wrapper*) new process_stream( ph, base, modules );
	_original_base = base;
	if (modules != NULL)
	{
		_name_filepath_long = new char[FILEPATH_SIZE];
		_name_filepath_long_size = _stream->get_long_name(_name_filepath_long, FILEPATH_SIZE);
		_name_filepath_short = new char[FILEPATH_SIZE];
		_name_filepath_short_size = _stream->get_short_name(_name_filepath_short, FILEPATH_SIZE);
	}

	if( _options->Verbose )
		fprintf( stdout, "INFO: Initialized header for module name %s.\n", this->get_name() );
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
	if (new_name == NULL)
		return;
	const SIZE_T length = strnlen_s(new_name, MAX_PE_IMAGE_SIZE);
	if (length >= MAX_PE_IMAGE_SIZE)
		return;
	std::unique_ptr<char[]> name(new char[length + 1]);
	memcpy(name.get(), new_name, length + 1);
	// Set name to sue for this module
	if( _name_filepath_short != NULL )
		delete[] _name_filepath_short;

	// Localize
	_name_filepath_short = name.release();
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
			if( _image != NULL && range_fits(_image_size, offset, read_size) )
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
}

unsigned __int64 pe_header::_hash_short_asm(SIZE_T offset)
{
	unsigned __int64 result = 0;

	if (offset == 0 || !_options->EntryPointHash)
	{
		// Invalid entry point for hashing or code hashing is disabled
		return result;
	}

	// Load the 8 bytes at the offset as the short hash
	if (_image != NULL && range_fits(_image_size, offset, 8))
	{
		result = *((unsigned __int64*)(_image + offset));

		// Require at least 2 different byte values to be valid
		if ( ((result ^ (result << 8)) & 0xffffffffffffff00) == 0)
		{
			// All the same bytes, so it isn't valid
			return 0;
		}
	}

	return result;
}

unsigned __int64 pe_header::_hash_asm(SIZE_T offset)
{
	// Calculate the entry-point hash given the specified entry point
	unsigned __int64 result = 0;

	if (offset == 0 || !_options->EntryPointHash)
	{
		// Invalid entry point for hashing or code hashing is disabled
		return result;
	}
	
	if (this->_parsed_pe_32 || this->_parsed_pe_64)
	{
		// Partial hash of the code at the entry point. A database of these hashes for known modules can be
		// used to attempt to recover the original entry-point of dumped modules with tampered entry points.

		// First load the disassembly mode
		NMD_X86_MODE mode;
		if (_parsed_pe_32)
		{
			mode = NMD_X86_MODE_32;
		}
		else
		{
			mode = NMD_X86_MODE_64;
		}

		// Hashing logic:
		//  Hash opcodes + prefix
		//  Hash the first 100 instructions or until a ret is hit. If a ret is hit before 30 instructions are processed,
		//	it will continue blindly processing until 30 instructions are processed.

		bool more;
		bool return_hit = false;
		NMD_X86Instruction inst;
		int inst_count = 0;
		do
		{
			more = false;
			if (_image != NULL && range_fits(_image_size, offset, 20))
			{
				// Disassemble
				more = true;
				if (nmd_x86_decode_buffer(_image + offset, 20, &inst, mode, NMD_X86_DECODER_FLAGS_MINIMAL))
				{
					// Hash in this opcode
					result = (result + inst.opcode + (inst.opcode << 8) + (inst.opcode << 16) + (inst.opcode << 24)) ^ 0x8ADFA91F8ADFA91F;
					result = _rotl64(result, 0x13);

					// Hash in the opcode prefix
					result = (result + inst.prefixes + (inst.prefixes << 16)) ^ 0x18F31A228FA9B17A;
					result = _rotl64(result, 0x17);

					// Check if it was a function return
					if (inst.group == NMD_GROUP_RET)
					{
						return_hit = true;
					}

					offset += inst.length;
				}
				else
				{
					// Failed to disassemble. Hash this information and skip to the next byte.
					result = result ^ 0xCDA13B89DB31AF13;
					result = _rotl64(result, 0x18);

					offset++;
				}

				inst_count++;
			}

			if (more)
			{
				if (return_hit && inst_count >= EP_HASH_OPCODES_MIN)
				{
					// End if we've hit a return instruction and we've finished hashing at least 30 instructions
					more = false;
				}

				if (inst_count >= EP_HASH_OPCODES_MAX)
				{
					// We hash at most 100 instructions
					more = false;
				}
			}
		} while (more);

		if (inst_count >= EP_HASH_OPCODES_MIN)
		{
			return result;
		}
	}
	
	return 0; // Not a valid hash

}

bool pe_header::process_hash_ep()
{
	// Build the hash the entry point code
	this->_unique_hash_ep = 0;
	this->_unique_hash_ep_short = 0;
	
	// Calculate the entry-point hash given the specified entry point
	unsigned __int64 result = 0;

	if (this->_parsed_pe_32 || this->_parsed_pe_64)
	{
		// Partial hash of the code at the entry point. A database of these hashes for known modules can be
		// used to attempt to recover the original entry-point of dumped modules with tampered entry points.

		// First load the entry point
		SIZE_T offset = 0;
		NMD_X86_MODE mode;
		if (_parsed_pe_32)
		{
			offset = _header_pe32->OptionalHeader.AddressOfEntryPoint; // rva
		}
		else
		{
			offset = _header_pe64->OptionalHeader.AddressOfEntryPoint;
		}

		// Load the 8 bytes at the entrypoint as the short hash
		unsigned __int64 hash = _hash_short_asm(offset);
		if (hash != 0)
		{
			_unique_hash_ep_short = hash;

			// Load the hash of the code at the entry point
			hash = _hash_asm(offset);
			if (hash != 0)
			{
				this->_unique_hash_ep = hash;
				return true;
			}
		}

		
	}

	// Failed to parse an entry point hash
	return false;
}


bool pe_header::write_image( char* filename )
{
	return write_new_dump(filename, _disk_image, _disk_image_size);
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
	unordered_set<string> import_libraries;

	if( _options->Verbose )
			printf( "INFO: Building import information.\n" );
	
	IMPORT_SUMMARY result;
	result.COUNT_UNIQUE_IMPORT_ADDRESSES = 0;
	result.COUNT_UNIQUE_IMPORT_LIBRARIES = 0;
	result.HASH_GENERIC = 0;
	result.HASH_SPECIFIC = 0;

	size_t hash_generic = 0x1a78ac10;
	size_t hash_specific = 0x1a78ac10;

	hash<string> hasher;
	
	if( this->_parsed_sections && exports != NULL && size_limit >= 4 )
	{
		const SIZE_T limit = static_cast<SIZE_T>((std::min<unsigned __int64>)(_image_size, size_limit));
		// Add matches to exports in this process
		unsigned __int32 cand32_last = 0;
		unsigned __int64 cand64_last = 0;
		for(SIZE_T offset = 0; range_fits(limit, offset, sizeof(DWORD)); offset += sizeof(DWORD))
		{
			// Check if this 4-gram or 8-gram points to an export
			unsigned __int32 cand32 = *((__int32*)(_image + offset));

			if (cand32 != cand32_last)
			{
				if (const export_entry* found = exports->lookup(cand32))
				{
					const export_entry& entry = *found;

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
						else
						{
							const size_t ordinal_hash = hasher("#" + std::to_string(entry.ord));
							hash_generic ^= ordinal_hash;
							hash_specific ^= ordinal_hash;
						}
						if (entry.library_name != NULL)
						{
							import_libraries.insert(entry.library_name);
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
			
			if (!range_fits(limit, offset, sizeof(unsigned __int64)))
				continue;
			unsigned __int64 cand64 = *((unsigned __int64*)(_image + offset));
			if (cand64 != cand64_last && cand64 > 0xffffffff)
			{
				if (const export_entry* found = exports->lookup(cand64))
				{
					const export_entry& entry = *found;

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
						else
						{
							const size_t ordinal_hash = hasher("#" + std::to_string(entry.ord));
							hash_generic ^= ordinal_hash;
							hash_specific ^= ordinal_hash;
						}
						if (entry.library_name != NULL)
						{
							import_libraries.insert(entry.library_name);
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
	result.COUNT_UNIQUE_IMPORT_LIBRARIES = import_libraries.size();

	if( _options->Verbose )
	{
		printf( "INFO: Finished building import information:\n" );
		printf( "INFO: Count Unique Import Addresses = %i\n", result.COUNT_UNIQUE_IMPORT_ADDRESSES );
		printf( "INFO: Count Unique Import Libraries = %i\n", result.COUNT_UNIQUE_IMPORT_LIBRARIES );
		printf( "INFO: Generic Hash = 0x%llX\n", result.HASH_GENERIC );
		printf( "INFO: Specific Hash = 0x%llX\n", result.HASH_SPECIFIC );
	}

	return result;
}

unsigned __int64 pe_header::get_hash()
{
	if( _unique_hash == 0 )
		process_hash();
	return _unique_hash;
}

unsigned __int64 pe_header::get_hash_ep()
{
	if (_unique_hash_ep == 0)
		process_hash_ep();
	return _unique_hash_ep;
}

unsigned __int64 pe_header::get_hash_ep_short()
{
	if (_unique_hash_ep_short == 0)
		process_hash_ep();
	return _unique_hash_ep_short;
}


bool pe_header::build_pe_header( __int64 size, bool amd64 )
{
	return build_pe_header( size, amd64, 99 ); // Build it with as many sections as we can.
}

bool pe_header::build_pe_header( __int64 size, bool amd64, int num_sections_limit )
{
	_clear_header();
	if (!image_size_fits(size) || num_sections_limit <= 0)
		return _reject_size();
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
				_header_pe32->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_32BIT_MACHINE;
			else
				_header_pe32->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
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
				_header_pe64->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;
			else
				_header_pe64->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
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
			if (!image_size_fits(est_size) || est_size == 0 ||
				!range_fits(MAX_PE_IMAGE_SIZE, static_cast<SIZE_T>(image_size), static_cast<SIZE_T>(est_size)))
			{
				_clear_header();
				return _reject_size();
			}
			
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
				printf("%s: size %x\n", name, image_size);

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
	_clear_header();
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loading PE header for %s.\n", this->get_name() );

	if( _stream != NULL )
	{
		// Request the block size of the first region
		const unsigned __int64 block_size = _stream->block_size(0);
		if (block_size > MAX_PE_IMAGE_SIZE)
			return _reject_size();
		_raw_header_size = static_cast<SIZE_T>(min(block_size, 16ULL * 1024 * 1024));
		if (_raw_header_size == 0)
			return false;
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
						if (_header_dos->e_lfanew < 0 ||
							!range_fits(_raw_header_size, static_cast<SIZE_T>(_header_dos->e_lfanew), sizeof(IMAGE_NT_HEADERS64)))
							return false;
						unsigned char* base_pe = _header_dos->e_lfanew + _raw_header;
						
						if( _test_read( _raw_header, _raw_header_size, base_pe, sizeof(IMAGE_NT_HEADERS64) ) )
						{
							// We are unsure if we need to process this as a 32bit or 64bit PE header, lets figure it out.
							// The first part is independent of the 32 or 64 bit definition.
							if ( ((IMAGE_NT_HEADERS32*)base_pe)->Signature == 0x4550 && ((IMAGE_NT_HEADERS32*)base_pe)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
							{
								// 32bit module
								this->_header_pe32 = ((IMAGE_NT_HEADERS32*) base_pe);
								this->_parsed_pe_32 = true;
								if( _options->Verbose )
									fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\n", this->get_name(), this->somewhat_parsed() );
								return true;
							}
							else if( ((IMAGE_NT_HEADERS64*)base_pe)->Signature == 0x4550 && ((IMAGE_NT_HEADERS64*)base_pe)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
							{
								// 64bit module
								this->_header_pe64 = ((IMAGE_NT_HEADERS64*) base_pe);
								this->_parsed_pe_64 = true;
								if( _options->Verbose )
									fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\n", this->get_name(), this->somewhat_parsed() );
								return true;
							}
							else
							{
								// error
								if (_options->Verbose)
									fprintf(stdout, "INFO: Invalid PE header for %s. Somewhat parsed: %d\n", this->get_name(), this->somewhat_parsed());
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
			fprintf( stderr, "INFO: Invalid stream.\n" );
	}
	
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loaded PE header for %s. Somewhat parsed: %d\n", this->get_name(), this->somewhat_parsed() );

	return false;
}

bool pe_header::process_sections( )
{
	_clear_images();
	if (!_parsed_pe_32 && !_parsed_pe_64)
		return false;
	const WORD optional_size = _parsed_pe_32 ? _header_pe32->FileHeader.SizeOfOptionalHeader : _header_pe64->FileHeader.SizeOfOptionalHeader;
	const SIZE_T minimum_optional_size = _parsed_pe_32 ? sizeof(IMAGE_OPTIONAL_HEADER32) : sizeof(IMAGE_OPTIONAL_HEADER64);
	if (optional_size < minimum_optional_size)
		return _reject_size();
	const SIZE_T section_offset = static_cast<SIZE_T>(_header_dos->e_lfanew) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + optional_size;
	WORD section_count = _parsed_pe_32 ? _header_pe32->FileHeader.NumberOfSections : _header_pe64->FileHeader.NumberOfSections;
	const DWORD declared_image_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfImage : _header_pe64->OptionalHeader.SizeOfImage;
	const DWORD headers_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfHeaders : _header_pe64->OptionalHeader.SizeOfHeaders;
	if (section_count == 0 || !range_fits(_raw_header_size, section_offset, sizeof(IMAGE_SECTION_HEADER)) ||
		declared_image_size > MAX_PE_IMAGE_SIZE)
		return _reject_size();
	section_count = min(section_count, 0x100);
	if (!range_fits(_raw_header_size, section_offset, section_count * sizeof(IMAGE_SECTION_HEADER)))
		section_count = static_cast<WORD>((_raw_header_size - section_offset - 1) / sizeof(IMAGE_SECTION_HEADER));
	if (section_count == 0)
		return _reject_size();
	const IMAGE_SECTION_HEADER* sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(_raw_header + section_offset);
	const IMAGE_SECTION_HEADER& last = sections[section_count - 1];
	if (!range_fits(MAX_PE_IMAGE_SIZE, last.VirtualAddress, last.Misc.VirtualSize))
		return _reject_size();
	SIZE_T checked_image_size = last.VirtualAddress + last.Misc.VirtualSize;
	checked_image_size = max(checked_image_size, declared_image_size);
	if (checked_image_size == 0 || checked_image_size > MAX_PE_IMAGE_SIZE ||
		!range_fits(checked_image_size, 0, headers_size))
		return _reject_size();
	for (WORD i = 0; i < section_count; ++i)
	{
		const IMAGE_SECTION_HEADER& section = sections[i];
		if (!range_fits(checked_image_size, section.VirtualAddress, 0) ||
			!range_fits(MAX_PE_IMAGE_SIZE, section.VirtualAddress, section.Misc.VirtualSize) ||
			section.SizeOfRawData > MAX_PE_IMAGE_SIZE ||
			(_stream->file_alignment &&
				(!range_fits(checked_image_size, section.VirtualAddress, section.SizeOfRawData) ||
				 !range_fits(LONG_MAX, section.PointerToRawData, section.SizeOfRawData))))
			return _reject_size();
	}
	if( _options->Verbose )
		fprintf( stdout, "INFO: Loading sections for %s.\n", this->get_name() );

	if( this->_parsed_pe_32 )
	{
		// Attempt to parse the sections
		unsigned char* base_sections = _raw_header + section_offset;
		if( _header_pe32->FileHeader.NumberOfSections > 0x100 )
		{
			char* location = new char[FILEPATH_SIZE + 1];
			_stream->get_location(location, FILEPATH_SIZE + 1);
			fprintf( stderr, "WARNING: module '%s' at %s. Extremely large number of sections of 0x%x changed to 0x100 as part of sanity check.\n",
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
				fprintf( stderr, "WARNING: module '%s' at %s. Number of sections being changed from 0x%x to 0x%x such that it will fit within the PE header buffer.\n",
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
						fprintf( stdout, "INFO: %s\t#%i\t%.8s\t0x%x\t0x%x\n", this->get_name(), i, this->_header_sections[i].Name, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
					else
						fprintf( stdout, "INFO: %s\t#%i\tINVALID ADDRESS\t0x%x\t0x%x\n", this->get_name(), i, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
				}
			}


			const SIZE_T image_size = checked_image_size;
			
			// Perform a sanity check on the resulting image size
			if( image_size > MAX_PE_IMAGE_SIZE )
			{
				_clear_images();
				return _reject_size();
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
						fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header of size 0x%x. Was only able to read 0x%x bytes from this region.\n",this->get_name(), location, _header_pe32->OptionalHeader.SizeOfHeaders, num_read);
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
						if( range_fits(_image_size, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData) &&
							range_fits(LONG_MAX, this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData) )
						{
							// Read in this section
							if( !_stream->read( this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData,
								_image + (SIZE_T) this->_header_sections[i].VirtualAddress, &num_read ) && _options->Verbose )
							{
								char* location = new char[FILEPATH_SIZE + 1];
								_stream->get_location(location, FILEPATH_SIZE + 1);
								fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in section %i of size 0x%x. Was only able to read 0x%x bytes from this region.\n", this->get_name(), location, i, this->_header_sections[i].SizeOfRawData, num_read);
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
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in image at 0x%llX of size 0x%x. Was only able to read 0x%x bytes from this region.\n",this->get_name(), location, this->_stream->get_address(), _image_size, num_read);
					delete[] location;
				}
			}

			if( _options->Verbose )
				fprintf( stdout, "INFO: Loaded sections for %s with result: %d. %i sections found.\n", this->get_name(), this->_parsed_sections, ( this->_parsed_sections ? this->_num_sections : 0 )  );
			
			return true;
		}
	}
	else if( this->_parsed_pe_64 )
	{
		// Attempt to parse the sections
		unsigned char* base_sections = _raw_header + section_offset;
		if( _header_pe64->FileHeader.NumberOfSections > 0x100 )
		{
			char* location = new char[FILEPATH_SIZE + 1];
			_stream->get_location(location, FILEPATH_SIZE + 1);
			fprintf( stderr, "WARNING: module '%s' at %s. Extremely large number of sections of 0x%x changed to 0x100 as part of sanity check.\n",
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
				fprintf( stderr, "WARNING: module '%s' at %s. Number of sections being changed from 0x%x to 0x%x such that it will fit within the PE header buffer.\n",
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
					fprintf( stdout, "INFO: %s\t#%i\t%.8s\t0x%x\t0x%x\n", this->get_name(), i, this->_header_sections[i].Name, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData );
				}
			}

			const SIZE_T image_size = checked_image_size;
			
			// Perform a sanity check on the resulting image size
			if( image_size > MAX_PE_IMAGE_SIZE )
			{
				_clear_images();
				return _reject_size();
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
						fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in header of size 0x%x. Was only able to read 0x%x bytes from this region.\n",this->get_name(), location, _header_pe64->OptionalHeader.SizeOfHeaders, num_read);
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
						if( range_fits(_image_size, this->_header_sections[i].VirtualAddress, this->_header_sections[i].SizeOfRawData) &&
							range_fits(LONG_MAX, this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData) )
						{
							// Read in this section
							if( !_stream->read( this->_header_sections[i].PointerToRawData, this->_header_sections[i].SizeOfRawData,
								_image + (SIZE_T) this->_header_sections[i].VirtualAddress, &num_read ) && _options->Verbose )
							{
								char* location = new char[FILEPATH_SIZE + 1];
								_stream->get_location(location, FILEPATH_SIZE + 1);
								fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in section %i of size 0x%x. Was only able to read 0x%x bytes from this region.\n", this->get_name(), location, i, this->_header_sections[i].SizeOfRawData, num_read);
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
					fprintf( stderr, "WARNING: module '%s' at %s. Failed to read in image at 0x%llX of size 0x%x. Was only able to read 0x%x bytes from this region.\n",this->get_name(), location, this->_stream->get_address(), _image_size, num_read);
					delete[] location;
				}
			}

			if( _options->Verbose )
				fprintf( stdout, "INFO: Loaded sections for %s with result: %d. %i sections found.\n", this->get_name(), this->_parsed_sections, ( this->_parsed_sections ? this->_num_sections : 0 )  );

			return true;
		}
	}

	if( _options->Verbose )
		fprintf( stdout, "INFO: Failed to load sections for %s.\n", this->get_name() );

	return false;
}

bool pe_header::_append_import_section(DWORD rva, DWORD size)
{
	if (!range_fits(MAX_PE_IMAGE_SIZE, rva, size))
		return _reject_size();
	const SIZE_T next_header = reinterpret_cast<unsigned char*>(_header_sections + _num_sections) - _raw_header;
	const DWORD headers_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfHeaders : _header_pe64->OptionalHeader.SizeOfHeaders;
	if (_num_sections < 0x100 && range_fits(_raw_header_size, next_header, sizeof(IMAGE_SECTION_HEADER)) &&
		range_fits(headers_size, next_header, sizeof(IMAGE_SECTION_HEADER)) &&
		std::all_of(_raw_header + next_header, _raw_header + next_header + sizeof(IMAGE_SECTION_HEADER),
			[](unsigned char byte) { return byte == 0; }))
	{
		// Keep a preceding BSS tail virtual instead of materializing its zeros.
		IMAGE_SECTION_HEADER& section = _header_sections[_num_sections++];
		memset(&section, 0, sizeof(section));
		memcpy(section.Name, ".pdimp", 6);
		section.VirtualAddress = rva;
		section.Misc.VirtualSize = size;
		section.SizeOfRawData = size;
		section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		if (_parsed_pe_32)
			_header_pe32->FileHeader.NumberOfSections = static_cast<WORD>(_num_sections);
		else
			_header_pe64->FileHeader.NumberOfSections = static_cast<WORD>(_num_sections);
		return true;
	}
	IMAGE_SECTION_HEADER& last = _header_sections[_num_sections - 1];
	if (last.VirtualAddress > rva)
		return _reject_size();
	fprintf(stderr, "WARNING: No section-header space for reconstructed imports in '%s'; extending the last section.\n", get_name());
	last.Misc.VirtualSize = rva + size - last.VirtualAddress;
	last.SizeOfRawData = last.Misc.VirtualSize;
	return true;
}

bool pe_header::_writable_data_range(SIZE_T rva, SIZE_T width) const
{
	for (int i = 0; i < _num_sections; ++i)
	{
		const auto& section = _header_sections[i];
		const SIZE_T span = (std::max)(section.Misc.VirtualSize, section.SizeOfRawData);
		if (rva >= section.VirtualAddress && range_fits(span, rva - section.VirtualAddress, width))
			return (section.Characteristics & IMAGE_SCN_MEM_WRITE) != 0 &&
				(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0;
	}
	return false;
}

bool pe_header::_reexecution_import_ranges(std::vector<std::pair<SIZE_T, SIZE_T>>& ranges) const
{
	const auto& directory = _parsed_pe_32 ? _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] :
		_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
	std::vector<DWORD> delay_iats;
	if (directory.VirtualAddress != 0)
	{
		if (!range_fits(_image_size, directory.VirtualAddress, directory.Size))
			return false;
		bool terminated = false;
		for (SIZE_T offset = 0; range_fits(directory.Size, offset, 8 * sizeof(DWORD)); offset += 8 * sizeof(DWORD))
		{
			DWORD fields[8];
			memcpy(fields, _image + directory.VirtualAddress + offset, sizeof(fields));
			if (std::all_of(fields, fields + 8, [](DWORD value) { return value == 0; }))
			{
				terminated = true;
				break;
			}
			ULONGLONG iat = fields[3];
			if ((fields[0] & 1) == 0)
			{
				const ULONGLONG base = _original_base != NULL ? reinterpret_cast<uintptr_t>(_original_base) :
					(_parsed_pe_32 ? _header_pe32->OptionalHeader.ImageBase : _header_pe64->OptionalHeader.ImageBase);
				if (iat < base)
					return false;
				iat -= base;
			}
			const SIZE_T width = _parsed_pe_32 ? sizeof(DWORD) : sizeof(ULONGLONG);
			if (iat == 0 || iat > MAX_PE_IMAGE_SIZE || !range_fits(_image_size, static_cast<SIZE_T>(iat), width))
				return false;
			delay_iats.push_back(static_cast<DWORD>(iat));
		}
		if (!terminated)
			return false;
		std::sort(delay_iats.begin(), delay_iats.end());
	}
	for (int i = 0; i < _num_sections; ++i)
	{
		const auto& section = _header_sections[i];
		const SIZE_T span = (std::max)(section.Misc.VirtualSize, section.SizeOfRawData);
		const auto delay = std::lower_bound(delay_iats.begin(), delay_iats.end(), section.VirtualAddress);
		// The loader can protect delay-IAT sections before binding normal imports.
		if ((section.Characteristics & IMAGE_SCN_MEM_WRITE) && !(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) &&
			(delay == delay_iats.end() || *delay - section.VirtualAddress >= span))
			ranges.emplace_back(section.VirtualAddress, section.SizeOfRawData);
	}
	return true;
}

bool pe_header::_reexecution_cookie_rva(SIZE_T& rva) const
{
	rva = _image_size;
	const auto& config = _parsed_pe_32 ? _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] :
		_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
	const SIZE_T width = _parsed_pe_32 ? sizeof(DWORD) : sizeof(ULONGLONG);
	const SIZE_T field = _parsed_pe_32 ? offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, SecurityCookie) :
		offsetof(IMAGE_LOAD_CONFIG_DIRECTORY64, SecurityCookie);
	if (config.VirtualAddress == 0 || config.Size < field + width)
		return true;
	if (!range_fits(_image_size, config.VirtualAddress, config.Size))
		return false;
	DWORD declared_size = 0;
	memcpy(&declared_size, _image + config.VirtualAddress, sizeof(declared_size));
	if (declared_size < field + width)
		return true;
	ULONGLONG cookie = 0;
	memcpy(&cookie, _image + config.VirtualAddress + field, width);
	if (cookie == 0)
		return true;
	const ULONGLONG base = _original_base != NULL ?
		(_parsed_pe_32 ? static_cast<DWORD>(reinterpret_cast<uintptr_t>(_original_base)) : reinterpret_cast<uintptr_t>(_original_base)) :
		(_parsed_pe_32 ? _header_pe32->OptionalHeader.ImageBase : _header_pe64->OptionalHeader.ImageBase);
	if (cookie < base || cookie - base > MAX_PE_IMAGE_SIZE ||
		!range_fits(_image_size, static_cast<SIZE_T>(cookie - base), width) ||
		!_writable_data_range(static_cast<SIZE_T>(cookie - base), width))
		return false;
	rva = static_cast<SIZE_T>(cookie - base);
	return true;
}

bool pe_header::_pack_disk_image(const unsigned char* image, SIZE_T size, SIZE_T cookie_rva)
{
	const DWORD alignment = _parsed_pe_32 ? _header_pe32->OptionalHeader.SectionAlignment : _header_pe64->OptionalHeader.SectionAlignment;
	DWORD& headers_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfHeaders : _header_pe64->OptionalHeader.SizeOfHeaders;
	DWORD& file_alignment = _parsed_pe_32 ? _header_pe32->OptionalHeader.FileAlignment : _header_pe64->OptionalHeader.FileAlignment;
	DWORD& image_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfImage : _header_pe64->OptionalHeader.SizeOfImage;
	IMAGE_DATA_DIRECTORY* directories = _parsed_pe_32 ?
		_header_pe32->OptionalHeader.DataDirectory : _header_pe64->OptionalHeader.DataDirectory;
	const SIZE_T table_end = reinterpret_cast<unsigned char*>(_header_sections + _num_sections) - _raw_header;
	const __int64 aligned_headers = _section_align(static_cast<__int64>((std::max<SIZE_T>)(headers_size, table_end)), alignment);
	const __int64 aligned_image = _section_align(static_cast<__int64>(size), alignment);
	if (!image_size_fits(aligned_headers) || !image_size_fits(aligned_image) ||
		aligned_headers > static_cast<__int64>(size))
		return _reject_size();
	headers_size = static_cast<DWORD>(aligned_headers);
	file_alignment = alignment;
	image_size = static_cast<DWORD>(aligned_image);
	SIZE_T required_space = headers_size;
	std::vector<SIZE_T> copy_sizes;
	for (int i = 0; i < _num_sections; ++i)
	{
		IMAGE_SECTION_HEADER& section = _header_sections[i];
		const SIZE_T span = (std::max)(section.Misc.VirtualSize, section.SizeOfRawData);
		if (!range_fits(size, section.VirtualAddress, span) ||
			(span != 0 && section.VirtualAddress < headers_size))
			return _reject_size();
		SIZE_T initialized = span;
		while (initialized >= sizeof(SIZE_T))
		{
			SIZE_T word;
			memcpy(&word, image + section.VirtualAddress + initialized - sizeof(word), sizeof(word));
			if (word != 0)
				break;
			initialized -= sizeof(word);
		}
		while (initialized != 0 && image[section.VirtualAddress + initialized - 1] == 0)
			--initialized;
		for (int directory = 0; directory < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++directory)
		{
			if (directory == IMAGE_DIRECTORY_ENTRY_SECURITY)
				continue;
			const auto& entry = directories[directory];
			if (entry.VirtualAddress >= section.VirtualAddress &&
				range_fits(span, entry.VirtualAddress - section.VirtualAddress, entry.Size))
				initialized = (std::max<SIZE_T>)(initialized, entry.VirtualAddress - section.VirtualAddress + entry.Size);
		}
		const SIZE_T cookie_width = _parsed_pe_32 ? sizeof(DWORD) : sizeof(ULONGLONG);
		if (cookie_rva < _image_size && cookie_rva >= section.VirtualAddress &&
			range_fits(span, cookie_rva - section.VirtualAddress, cookie_width))
			initialized = (std::max)(initialized, cookie_rva - section.VirtualAddress + cookie_width);
		const __int64 raw_size = _section_align(static_cast<__int64>(initialized), alignment);
		if (!image_size_fits(raw_size) || !range_fits(MAX_PE_IMAGE_SIZE, required_space, static_cast<SIZE_T>(raw_size)))
			return _reject_size();
		section.SizeOfRawData = static_cast<DWORD>(raw_size);
		section.PointerToRawData = raw_size == 0 ? 0 : static_cast<DWORD>(required_space);
		if (raw_size != 0 && (section.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
			section.Characteristics = (section.Characteristics & ~IMAGE_SCN_CNT_UNINITIALIZED_DATA) | IMAGE_SCN_CNT_INITIALIZED_DATA;
		copy_sizes.push_back(initialized);
		required_space += static_cast<SIZE_T>(raw_size);
	}
	// Certificates use file offsets and are not mapped into the process.
	directories[IMAGE_DIRECTORY_ENTRY_SECURITY] = {};
	if (_parsed_pe_32)
		_header_pe32->OptionalHeader.CheckSum = 0;
	else
		_header_pe64->OptionalHeader.CheckSum = 0;
	_disk_image = new unsigned char[required_space]();
	_disk_image_size = required_space;
	memcpy(_disk_image, _raw_header, (std::min<SIZE_T>)(headers_size, _raw_header_size));
	for (int i = 0; i < _num_sections; ++i)
	{
		if (copy_sizes[i] != 0)
			memcpy(_disk_image + _header_sections[i].PointerToRawData,
				image + _header_sections[i].VirtualAddress, copy_sizes[i]);
	}
	const auto file_offset = [this, headers_size](DWORD rva, SIZE_T count, SIZE_T& offset) {
		if (rva < headers_size && range_fits(headers_size, rva, count))
		{
			offset = rva;
			return true;
		}
		for (int i = 0; i < _num_sections; ++i)
		{
			const auto& section = _header_sections[i];
			if (rva >= section.VirtualAddress && range_fits(section.SizeOfRawData, rva - section.VirtualAddress, count))
			{
				offset = section.PointerToRawData + rva - section.VirtualAddress;
				return true;
			}
		}
		return false;
	};
	const auto& debug = directories[IMAGE_DIRECTORY_ENTRY_DEBUG];
	SIZE_T debug_offset = 0;
	if (debug.Size != 0 && file_offset(debug.VirtualAddress, debug.Size, debug_offset))
	{
		for (SIZE_T i = 0; i < debug.Size / sizeof(IMAGE_DEBUG_DIRECTORY); ++i)
		{
			auto& entry = *reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(_disk_image + debug_offset + i * sizeof(IMAGE_DEBUG_DIRECTORY));
			SIZE_T data_offset = 0;
			entry.PointerToRawData = entry.AddressOfRawData != 0 &&
				file_offset(entry.AddressOfRawData, entry.SizeOfData, data_offset) ? static_cast<DWORD>(data_offset) : 0;
		}
	}
	if (cookie_rva < _image_size)
	{
		const SIZE_T width = _parsed_pe_32 ? sizeof(DWORD) : sizeof(ULONGLONG);
		SIZE_T offset = 0;
		if (!file_offset(static_cast<DWORD>(cookie_rva), width, offset))
		{
			fprintf(stderr, "WARNING: Cannot prepare '%s' for re-execution: GS cookie has no raw storage.\n", get_name());
			return false;
		}
		// The loader/CRT replaces this ABI sentinel with a fresh process-specific cookie.
		const ULONGLONG bootstrap = _parsed_pe_32 ? 0xbb40e64eULL : 0x2b992ddfa232ULL;
		memcpy(_disk_image + offset, &bootstrap, width);
		if (_options->Verbose)
			printf("INFO: Restored GS cookie bootstrap value for re-execution of '%s'.\n", get_name());
	}
	return true;
}

bool pe_header::process_disk_image( export_list* exports, pe_hash_database* hash_database)
{
	delete[] _disk_image;
	_disk_image = NULL;
	_disk_image_size = 0;
	if (!_parsed_sections || _num_sections <= 0 || _image == NULL || _image_size == 0 ||
		!image_size_fits(_image_size) || (!_parsed_pe_32 && !_parsed_pe_64))
		return false;
	const DWORD section_alignment = _parsed_pe_32 ? _header_pe32->OptionalHeader.SectionAlignment : _header_pe64->OptionalHeader.SectionAlignment;
	const DWORD file_alignment = _parsed_pe_32 ? _header_pe32->OptionalHeader.FileAlignment : _header_pe64->OptionalHeader.FileAlignment;
	const DWORD header_size = _parsed_pe_32 ? _header_pe32->OptionalHeader.SizeOfHeaders : _header_pe64->OptionalHeader.SizeOfHeaders;
	if (section_alignment == 0 || file_alignment == 0 || section_alignment > MAX_PE_IMAGE_SIZE ||
		file_alignment > MAX_PE_IMAGE_SIZE || header_size > MAX_PE_IMAGE_SIZE ||
		(_options->ImportRec && exports == NULL))
		return _reject_size();
	for (int i = 0; i < _num_sections; ++i)
	{
		if (!range_fits(_image_size, _header_sections[i].VirtualAddress, 0) ||
			!range_fits(MAX_PE_IMAGE_SIZE, _header_sections[i].VirtualAddress, _header_sections[i].Misc.VirtualSize) ||
			_header_sections[i].SizeOfRawData > MAX_PE_IMAGE_SIZE)
			return _reject_size();
	}
	std::vector<std::pair<SIZE_T, SIZE_T>> import_ranges;
	std::vector<std::pair<SIZE_T, SIZE_T>> zero_fill;
	if (_options->Reexecution)
		for (int i = 0; i < _num_sections; ++i)
		{
			const auto& section = _header_sections[i];
			if ((section.Characteristics & IMAGE_SCN_MEM_WRITE) && !(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) &&
				section.Misc.VirtualSize > section.SizeOfRawData)
			{
				if (!range_fits(_image_size, section.VirtualAddress, section.Misc.VirtualSize))
					return _reject_size();
				zero_fill.emplace_back(section.VirtualAddress + section.SizeOfRawData,
					section.Misc.VirtualSize - section.SizeOfRawData);
			}
		}
	const auto restore_zero_fill = [&](unsigned char* image) {
		for (const auto& range : zero_fill)
			memset(image + range.first, 0, range.second);
	};
	SIZE_T cookie_rva = _image_size;
	if (_options->Reexecution && !_reexecution_cookie_rva(cookie_rva))
	{
		fprintf(stderr, "WARNING: Cannot prepare '%s' for re-execution: invalid load configuration or GS cookie location.\n", get_name());
		return false;
	}
	if (_options->Reexecution && _options->ImportRec && !_reexecution_import_ranges(import_ranges))
	{
		fprintf(stderr, "WARNING: Cannot prepare '%s' for re-execution: invalid delay-import metadata.\n", get_name());
		return false;
	}
	const auto allow_import = [&](SIZE_T rva, SIZE_T width) {
		if (cookie_rva < _image_size && rva < cookie_rva + width && cookie_rva < rva + width)
			return false;
		return !_options->Reexecution || std::any_of(import_ranges.begin(), import_ranges.end(),
			[&](const std::pair<SIZE_T, SIZE_T>& range) {
				return rva >= range.first && range_fits(range.second, rva - range.first, width);
			});
	};
	if( this->_parsed_sections )
	{
		if( this->_parsed_pe_32 )
		{
			// Re-build the Original Entry Point OEP if it looks to be not valid
			if (hash_database != NULL && (_header_pe32->OptionalHeader.AddressOfEntryPoint == 0 ||
				_header_pe32->OptionalHeader.AddressOfEntryPoint == 0x2000 ||
				!range_fits(_image_size, _header_pe32->OptionalHeader.AddressOfEntryPoint, 20) ||
				_options->ForceReconstructEntryPoint))
			{
				printf("INFO: Re-building entrypoint. Original entrypoint invalid: %x\n", _header_pe32->OptionalHeader.AddressOfEntryPoint);

				// The entry-point looks invalid, search for candidates to reconstruct it
				unsigned __int64 best_entrypoint = 0;
				const auto entrypoints = hash_database->snapshot_entrypoints();

				for (__int64 offset = 0x1000; !entrypoints->short_hashes.empty() && _image_size >= 8 && offset < _image_size - 8; offset += 1)
				{
					// Check if this is a possible entrypoint
					unsigned __int64 cand = *((__int64*)(_image + offset));

					// Lookup the address
					if (cand >= entrypoints->minimum && cand <= entrypoints->maximum && entrypoints->short_hashes.count(cand) != 0)
					{
						// This is a possible entrypoint, this is a weak correlation but we'll use it if it's all we have
						if (best_entrypoint == 0)
						{
							best_entrypoint = offset;
						}
						if( _options->Verbose )
							printf("INFO: Possible entrypoint found (weak): %x\n", offset);

						// Validate that the full hash matches a known entrypoint
						cand = _hash_asm(offset);
						if (entrypoints->full.count(cand) != 0)
						{
							best_entrypoint = offset;
							printf("INFO: Possible entrypoint found (strong): %x\n", offset);
							if (!_options->Verbose)
								break;
						}
					}
				}

				// Update the entrypoint
				if (best_entrypoint != 0)
				{
					_header_pe32->OptionalHeader.AddressOfEntryPoint = best_entrypoint;
					printf("INFO: Updated entrypoint to: %x\n", best_entrypoint);
				}
			}

			// Reconstruct PE imports aggressively using our knowledge of all the exports addresses in this process
			// Technique:
			//   1. 'exports' defines all valid export addresses in this process
			//   2. Find any binary that points to a valid export in this rpocess
			//   3. Add a new section for the new HintName Array and Import Address Table
			//   4. For each binary patch found above, add a HintName and ImportAddress point so the loads
			//      will recognize it correctly for analysis (IDA).
			unsigned char* larger_image;
			std::unique_ptr<unsigned char[]> larger_image_storage;
			__int64 larger_image_size;
			if( _options->ImportRec )
			{
				// Start the with the original import descriptor list
				std::unique_ptr<pe_imports> peimp(new pe_imports( _image, _image_size, _header_import_descriptors, false ));
				if (!peimp->valid())
					return _reject_size();
				
				// Add matches to exports in this process
				int count = 0;
				unsigned __int64 cand_last = 0;
				for (SIZE_T offset = 0; range_fits(_image_size, offset, sizeof(DWORD)); offset += sizeof(DWORD))
				{
					unsigned __int32 cand = 0;
					memcpy(&cand, _image + offset, sizeof(cand));

					const export_entry* entry = cand_last != cand ? exports->lookup(cand) : NULL;
					if (entry != NULL && allow_import(offset, sizeof(DWORD)))
					{
						// Add this to be reconstructed as an import
						peimp->add_fixup(*entry, offset, this->_parsed_pe_64);
						if (!peimp->valid())
							return _reject_size();
						count++;
					}
					else if (entry == NULL)
					{
						cand_last = cand;
					}
				}
				if( _options->Verbose )
					printf( "INFO: Reconstructing %i imports.\n", count );
				
				// Increase the image size for a new section
				__int64 descriptor_size = 0;
				__int64 data_size = 0;
				peimp->get_table_size( descriptor_size, data_size );
				if (!image_size_fits(descriptor_size) || !image_size_fits(data_size) ||
					!image_size_fits(data_size + descriptor_size))
					return _reject_size();
				const __int64 new_section_size = _section_align(data_size + descriptor_size, section_alignment);
				const __int64 section_start = _section_align(static_cast<__int64>(_image_size), section_alignment);
				larger_image_size = section_start + new_section_size;
				if (new_section_size <= 0 || section_start < static_cast<__int64>(_image_size) ||
					!image_size_fits(larger_image_size))
					return _reject_size();
				larger_image_storage.reset(new unsigned char[static_cast<SIZE_T>(larger_image_size)]);
				larger_image = larger_image_storage.get();
				memset(larger_image + _image_size, 0, larger_image_size - _image_size);
				memcpy(larger_image, _image, _image_size);
				restore_zero_fill(larger_image);

				if( _options->Verbose )
					printf( "INFO: Writing added import table.\n" );
				
				// Write to the new section
				if (!peimp->build_table(larger_image + static_cast<SIZE_T>(section_start),
					new_section_size, section_start, 0, descriptor_size) ||
					!_append_import_section(static_cast<DWORD>(section_start), static_cast<DWORD>(new_section_size)))
					return _reject_size();
				
				if( _options->Verbose )
					printf( "INFO: Updating import data directory.\n" );

				// Update the PE header to refer to it
				_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = static_cast<DWORD>(section_start);
				_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = static_cast<DWORD>(descriptor_size);
				
			}
			else
			{
				larger_image_size = _image_size;
				larger_image = _image;
				if (!zero_fill.empty())
				{
					larger_image_storage.reset(new unsigned char[_image_size]);
					larger_image = larger_image_storage.get();
					memcpy(larger_image, _image, _image_size);
					restore_zero_fill(larger_image);
				}
			}
			
			if( _original_base != 0 )
			{
				// Adjust the preferred image base, this way the relocations doesn't have to be fixed
				_header_pe32->OptionalHeader.ImageBase = (DWORD) _original_base;
			}

			return _pack_disk_image(larger_image, static_cast<SIZE_T>(larger_image_size), cookie_rva);
		}
		else if( this->_parsed_pe_64 )
		{
			// Re-build the Original Entry Point OEP if it looks to be not valid
			if (hash_database != NULL && (_header_pe64->OptionalHeader.AddressOfEntryPoint == 0 ||
				_header_pe64->OptionalHeader.AddressOfEntryPoint == 0x2000 ||
				!range_fits(_image_size, _header_pe64->OptionalHeader.AddressOfEntryPoint, 20) ||
				_options->ForceReconstructEntryPoint))
			{
				printf("INFO: Re-building entrypoint. Original entrypoint invalid: %x\n", _header_pe64->OptionalHeader.AddressOfEntryPoint);

				// The entry-point looks invalid, search for candidates to reconstruct it
				unsigned __int64 best_entrypoint = 0;
				const auto entrypoints = hash_database->snapshot_entrypoints();

				for (__int64 offset = 0x1000; !entrypoints->short_hashes.empty() && _image_size >= 8 && offset < _image_size - 8; offset += 1)
				{
					// Check if this is a possible entrypoint
					unsigned __int64 cand = *((__int64*)(_image + offset));

					// Lookup the address
					if (cand >= entrypoints->minimum && cand <= entrypoints->maximum && entrypoints->short_hashes.count(cand) != 0)
					{
						// This is a possible entrypoint, this is a weak correlation but we'll use it if it's all we have
						if (best_entrypoint == 0)
						{
							best_entrypoint = offset;
						}
						if (_options->Verbose)
							printf("INFO: Possible entrypoint found (weak): %x\n", offset);

						// Validate that the full hash matches a known entrypoint
						cand = _hash_asm(offset);
						if (entrypoints->full.count(cand) != 0)
						{
							best_entrypoint = offset;
							printf("INFO: Possible entrypoint found (strong): %x\n", offset);
							if (!_options->Verbose)
								break;
						}
					}
				}

				// Update the entrypoint
				if (best_entrypoint != 0)
				{
					_header_pe64->OptionalHeader.AddressOfEntryPoint = best_entrypoint;
					printf("INFO: Updated entrypoint to: %x\n", best_entrypoint);
				}
			}

			// Reconstruct PE imports aggressively using our knowledge of all the exports addresses in this process
			// Technique:
			//   1. 'exports' defines all valid export addresses in this process
			//   2. Find any binary that points to a valid export in this rpocess
			//   3. Add a new section for the new HintName Array and Import Address Table
			//   4. For each binary patch found above, add a HintName and ImportAddress point so the loads
			//      will recognize it correctly for analysis (IDA).
			unsigned char* larger_image;
			std::unique_ptr<unsigned char[]> larger_image_storage;
			__int64 larger_image_size;
			if( _options->ImportRec )
			{
				// Start the with the original import descriptor list
				std::unique_ptr<pe_imports> peimp(new pe_imports( _image, _image_size, _header_import_descriptors, true ));
				if (!peimp->valid())
					return _reject_size();
				
				// Add matches to exports in this process
				int count = 0;
				unsigned __int64 cand_last = 0;
				for (SIZE_T offset = 0; range_fits(_image_size, offset, sizeof(unsigned __int64)); offset += sizeof(DWORD))
				{
					unsigned __int64 cand = 0;
					memcpy(&cand, _image + offset, sizeof(cand));

					const export_entry* entry = cand_last != cand ? exports->lookup(cand) : NULL;
					if (entry != NULL && allow_import(offset, sizeof(ULONGLONG)))
					{
						// Add this to be reconstructed as an import
						peimp->add_fixup(*entry, offset, this->_parsed_pe_64);
						if (!peimp->valid())
							return _reject_size();
						count++;
					}
					else if (entry == NULL)
					{
						cand_last = cand;
					}
				}
				if( _options->Verbose )
					printf( "INFO: Reconstructing %i imports.\n", count );
				
				// Increase the image size for a new section
				__int64 descriptor_size = 0;
				__int64 data_size = 0;
				peimp->get_table_size( descriptor_size, data_size );
				if (!image_size_fits(descriptor_size) || !image_size_fits(data_size) ||
					!image_size_fits(data_size + descriptor_size))
					return _reject_size();
				const __int64 new_section_size = _section_align(data_size + descriptor_size, section_alignment);
				const __int64 section_start = _section_align(static_cast<__int64>(_image_size), section_alignment);
				larger_image_size = section_start + new_section_size;
				if (new_section_size <= 0 || section_start < static_cast<__int64>(_image_size) ||
					!image_size_fits(larger_image_size))
					return _reject_size();
				larger_image_storage.reset(new unsigned char[static_cast<SIZE_T>(larger_image_size)]);
				larger_image = larger_image_storage.get();
				memset(larger_image + _image_size, 0, larger_image_size - _image_size);
				memcpy(larger_image, _image, _image_size);
				restore_zero_fill(larger_image);

				if( _options->Verbose )
					printf( "INFO: Writing added import table.\n" );
				
				// Write to the new section
				if (!peimp->build_table(larger_image + static_cast<SIZE_T>(section_start),
					new_section_size, section_start, 0, descriptor_size) ||
					!_append_import_section(static_cast<DWORD>(section_start), static_cast<DWORD>(new_section_size)))
					return _reject_size();
				
				if( _options->Verbose )
					printf( "INFO: Updating import data directory.\n" );

				// Update the PE header to refer to it
				_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = static_cast<DWORD>(section_start);
				_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = static_cast<DWORD>(descriptor_size);
				
			}
			else
			{
				larger_image_size = _image_size;
				larger_image = _image;
				if (!zero_fill.empty())
				{
					larger_image_storage.reset(new unsigned char[_image_size]);
					larger_image = larger_image_storage.get();
					memcpy(larger_image, _image, _image_size);
					restore_zero_fill(larger_image);
				}
			}
			
				

			if( _original_base != 0 )
			{
				// Adjust the preferred image base, this way the relocations doesn't have to be fixed
				_header_pe64->OptionalHeader.ImageBase = reinterpret_cast<__int64> (_original_base);
			}

			return _pack_disk_image(larger_image, static_cast<SIZE_T>(larger_image_size), cookie_rva);
		}
	}
	return false;
}

bool pe_header::process_import_directory( )
{
	_header_import_descriptors = NULL;
	_header_import_descriptors_count = 0;
	if ((!_parsed_pe_32 && !_parsed_pe_64) || _image == NULL)
		return false;
	const SIZE_T imports_rva = _parsed_pe_32 ?
		_header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress :
		_header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (imports_rva == 0)
		return true;
	SIZE_T offset = imports_rva;
	bool terminated = false;
	while (range_fits(_image_size, offset, sizeof(IMAGE_IMPORT_DESCRIPTOR)))
	{
		const IMAGE_IMPORT_DESCRIPTOR* current = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(_image + offset);
		if (current->Characteristics == 0 && current->FirstThunk == 0 &&
			current->ForwarderChain == 0 && current->Name == 0)
		{
			terminated = true;
			break;
		}
		++_header_import_descriptors_count;
		offset += sizeof(IMAGE_IMPORT_DESCRIPTOR);
	}
	if (!terminated)
	{
		_header_import_descriptors_count = 0;
		return false;
	}
	if (_header_import_descriptors_count == 0)
		return true;
	_header_import_descriptors = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(_image + imports_rva);
	for (int i = 0; i < _header_import_descriptors_count; ++i)
	{
		SIZE_T destination = _header_import_descriptors[i].FirstThunk;
		SIZE_T source = _header_import_descriptors[i].OriginalFirstThunk;
		if (source == 0 || destination == 0 || source == destination)
			continue;
		const SIZE_T width = _parsed_pe_64 ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32);
		SIZE_T length = 0;
		bool terminated = false;
		while (range_fits(_image_size, source, length) &&
			range_fits(_image_size - source, length, width))
		{
			unsigned __int64 value = 0;
			memcpy(&value, _image + source + length, width);
			length += width;
			if (value == 0)
			{
				terminated = true;
				break;
			}
		}
		if (!terminated || !range_fits(_image_size, destination, length) ||
			(source < destination + length && destination < source + length))
		{
			fprintf(stderr, "WARNING: Invalid or overlapping import thunks in '%s'; preserving captured IAT bytes.\n", get_name());
			continue;
		}
		// Copy complete pointer-sized entries, including the terminator, only after validating both tables.
		memcpy(_image + destination, _image + source, length);
	}
	return true;
}

bool pe_header::process_export_directory( )
{
	delete _export_list;
	_export_list = NULL;
	_header_export_directory = NULL;

	if( (this->_parsed_pe_32 || this->_parsed_pe_64) && _image != NULL )
	{
		IMAGE_DATA_DIRECTORY directory;
		if( _parsed_pe_32 )
			directory = _header_pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		else
			directory = _header_pe64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		const SIZE_T exports_rva = directory.VirtualAddress;
		
		if( exports_rva != 0 && range_fits(_image_size, exports_rva, sizeof(IMAGE_EXPORT_DIRECTORY)) )
		{
			unsigned char* base_exports = _image + exports_rva;
			_header_export_directory = ((IMAGE_EXPORT_DIRECTORY*) base_exports);
			
			// Parse this export directory
			std::unique_ptr<export_list> exports(new export_list());
			if (!exports->add_exports(_image, _image_size, (__int64)_original_base, _header_export_directory, _parsed_pe_64, directory.Size))
			{
				fprintf(stderr, "WARNING: Invalid export table in '%s'; excluding it from import reconstruction.\n", get_name());
				return false;
			}
			_export_list = exports.release();
			
			return true;
		}
	}

	return false;
}

bool pe_header::_test_read( unsigned char* buffer, SIZE_T length, unsigned char* read_ptr, SIZE_T read_length )
{
	return test_read(buffer, length, read_ptr, read_length);
}

pe_header::~pe_header(void)
{
	if( this->_stream != NULL )
		delete this->_stream;
	_clear_header();
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
	const __int64 result = _section_align(static_cast<__int64>(address), alignment);
	return result < 0 ? 0 : static_cast<DWORD>(result);
}

__int64 pe_header::_section_align( __int64 address, DWORD alignment)
{
	if (!image_size_fits(address) || alignment == 0 || alignment > MAX_PE_IMAGE_SIZE)
		return -1;
	const __int64 remainder = address % alignment;
	if (remainder != 0)
	{
		const __int64 padding = alignment - remainder;
		if (padding > MAX_PE_IMAGE_SIZE - address)
			return -1;
		return address + padding;
	}
	return address;
}