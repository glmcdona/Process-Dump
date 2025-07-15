#pragma once

#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include "simple.h"
#include "module_list.h"


// A stream class that wraps reading from either a file or process memory offset.
class stream_wrapper
{
public:
	bool file_alignment;
	virtual SIZE_T block_size( long offset ) = 0;
	virtual bool read( long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read ) = 0;
	virtual SIZE_T get_short_name( char* out_name, SIZE_T out_name_size ) = 0;
	virtual SIZE_T get_long_name( char* out_name, SIZE_T out_name_size ) = 0;
	virtual SIZE_T get_location( char* out_name, SIZE_T out_name_size ) = 0;
	virtual __int64 get_address() = 0;
	virtual __int64 estimate_section_size( SIZE_T offset ) = 0;
	virtual DWORD get_region_characteristics( long offset ) = 0;
	virtual ~stream_wrapper() {}
	virtual void update_base( __int64 rva ) = 0;
};


class file_stream : public stream_wrapper
{
	char* _filename;

	bool opened;
	FILE* fh;
public:

	file_stream(char* filename)
	{
		// Localize the filename
		_filename = new char[ strlen(filename) + 1 ];
		strcpy( _filename, filename );

		// Set the input stream as a file
		fh = fopen(filename, "rb");
		file_alignment = true;

		if( fh != NULL )
			opened = true;
		else
		{
			PrintLastError(L"Failed to open file.");
			opened = false;
		}
	}

	virtual void update_base( __int64 rva )
	{
			// nothing
	}

	virtual __int64 get_address( )
	{
		return 0;
	}

	virtual __int64 estimate_section_size( SIZE_T offset )
	{
		return 0;
	}

	virtual SIZE_T get_location( char* out_name, SIZE_T out_name_size )
	{
		return get_long_name( out_name, out_name_size );
	}

	virtual SIZE_T get_long_name( char* out_name, SIZE_T out_name_size )
	{
		// Return the path of this file on disk
		SIZE_T length = strlen(_filename) + 1;
		if( length > out_name_size )
			length = out_name_size;
		memcpy( out_name, _filename, length );
		out_name[length-1] = 0;

		return length - 1;
	}

	virtual SIZE_T get_short_name( char* out_name, SIZE_T out_name_size )
	{
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char short_name[_MAX_FNAME + _MAX_EXT + 1];
		_splitpath( _filename, NULL, NULL, fname, ext );
		sprintf( short_name, "%s.%s", fname, ext );

		SIZE_T length = strlen(short_name) + 1;
		if( length > out_name_size )
			length = out_name_size;
		memcpy( out_name, short_name, length );
		out_name[length-1] = 0;

		return length - 1;
	}


	virtual SIZE_T block_size( long offset )
	{
		if( opened )
		{
			if( !fseek( fh, 0, SEEK_END) )
				return (SIZE_T)ftell( fh ) - offset;
			else
				PrintLastError(L"Seek failed.");
		}
		return 0;
	}

	virtual DWORD get_region_characteristics( long offset )
	{
		return IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	}

	virtual bool read( long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read )
	{
		*out_read = 0;

		if( opened )
		{
			if( !fseek( fh, offset, SEEK_SET) )
			{
				*out_read = fread( output, 1, size, fh );
				if( *out_read == size )
					return true;
			}
		}
		return false;
	}

	~file_stream(void)
	{
		if( opened )
			fclose( fh );
		if( _filename != NULL )
			delete[] _filename;
	}
};



class process_stream : public stream_wrapper
{
	bool opened;
	HANDLE ph;

	char* _long_name;
	char* _short_name;
	
	void init( HANDLE ph, void* base, module_list* modules )
	{
		file_alignment = false;
		this->ph = ph;
		_long_name = NULL;
		_short_name = NULL;
		if( ph != NULL )
		{
			opened = true;
			this->base = base;

			// Copy this long and short name
			unordered_map<unsigned __int64, module*>::const_iterator item = modules->_modules.find( (unsigned __int64) base );
			if( item != modules->_modules.end() )
			{
				_long_name = new char[260];
				_short_name = new char[256];
				strcpy( _long_name, ((module*)item->second)->full_name );
				strcpy( _short_name, ((module*)item->second)->short_name );
			}
		}
		else
			opened = false;
	}

public:
	void* base;

	process_stream(HANDLE ph, void* base)
		:base(NULL)
	{
		_long_name = NULL;
		_short_name = NULL;
		file_alignment = false;
		this->ph = ph;
		if( ph != NULL )
		{
			opened = true;
			this->base = base;
		}
		else
			opened = false;
	}

	process_stream(HANDLE ph, void* base, module_list* modules )
	{
		init( ph, base, modules );
	}

	process_stream(DWORD pid, module_list* modules)
		:base(NULL)
	{
		_long_name = NULL;
		_short_name = NULL;

		// Try to open the specified process pid and use the main module as the base
		file_alignment = false;
		ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
		opened = false;
		
		if( ph != NULL )
		{
			HANDLE hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
			if( hSnapshot != INVALID_HANDLE_VALUE )
			{
				MODULEENTRY32 tmpModule;
				tmpModule.dwSize = sizeof(MODULEENTRY32);
				if( Module32First(hSnapshot, &tmpModule) )
				{
					opened = true;
					this->base = tmpModule.modBaseAddr;
				}
				CloseHandle( hSnapshot );				

				init( ph, base, modules );
			}
			else
			{
				if( GetLastError() == 299 )
					fprintf(stderr, "ERROR: Unable to open process PID 0x%x since it is a 64 bit process and this tool is running as a 32 bit process.\n", pid);
				else
					PrintLastError(L"create_process_stream CreateToolhelp32Snapshot");
			}
		}
		else
		{
			fprintf(stderr, "Failed to open process with PID 0x%x:\n", pid );
			PrintLastError(L"\tcreate_process_stream");
		}
	}

	process_stream(DWORD pid, void* base, module_list* modules )
		:base(NULL)
	{
		// Try to open the specified process pid
		_long_name = NULL;
		_short_name = NULL;
		file_alignment = false;
		opened = false;
		ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
		
		if( ph != NULL )
		{
			init( ph, base, modules );
		}
		else
		{
			fprintf(stderr, "Failed to open process with PID 0x%x:\n", pid );
			PrintLastError(L"\tcreate_process_stream");
		}
	}

	virtual SIZE_T get_location( char* out_name, SIZE_T out_name_size )
	{
		char hex[16 + 2 + 1]; // Max space required
		SIZE_T hexLength = sprintf( hex, "0x%llX", (__int64) this->base );

		if( hexLength < out_name_size )
		{
			memcpy( out_name, hex, hexLength );
			out_name[hexLength] = 0;
			return hexLength;
		}

		return 0;
	}

	virtual SIZE_T get_short_name( char* out_name, SIZE_T out_name_size )
	{
		if( _short_name != NULL )
		{
			SIZE_T length = strlen( _short_name ) + 1;
			if( length > out_name_size )
				length = out_name_size;
			memcpy( out_name, _short_name, length );
			out_name[length-1] = 0;

			return length;
		}
		return 0;
	}

	virtual SIZE_T get_long_name( char* out_name, SIZE_T out_name_size )
	{
		if( _long_name != NULL )
		{
			SIZE_T length = strlen( _long_name ) + 1;
			if( length > out_name_size )
				length = out_name_size;
			memcpy( out_name, _long_name, length );
			out_name[length-1] = 0;

			return length;
		}

		return 0;
	}

	virtual SIZE_T block_size( long offset )
	{
		if( opened )
		{
			_MEMORY_BASIC_INFORMATION64 mbi;
			__int64 blockSize = VirtualQueryEx(ph, (LPCVOID) ((unsigned char*)base + (SIZE_T)offset), (PMEMORY_BASIC_INFORMATION) &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

			if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
			{
				return (SIZE_T)(mbi.RegionSize - offset);
			}
			else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
			{
				_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
				return (SIZE_T)mbi32->RegionSize - offset;
			}
			else if( blockSize == 0 )
			{
				PrintLastError(L"VirtualQueryEx query block size");
			}
		}
		return 0;
	}

	virtual DWORD get_region_characteristics( long offset )
	{
		DWORD characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		if( opened )
		{
			_MEMORY_BASIC_INFORMATION64 mbi;
			SIZE_T blockSize = VirtualQueryEx(ph, (LPCVOID) ((unsigned char*)base + (SIZE_T)offset), (_MEMORY_BASIC_INFORMATION*) &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

			if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
			{
				if( mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					if( mbi.AllocationProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE  ) )
						characteristics |= IMAGE_SCN_MEM_EXECUTE;
				}
			}
			else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
			{
				_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
				if( mbi32->State == MEM_COMMIT && !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					if( mbi32->AllocationProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE  ) )
						characteristics |= IMAGE_SCN_MEM_EXECUTE;
				}
			}
		}
		
		return characteristics;
	}

	virtual __int64 estimate_section_size( SIZE_T offset )
	{
		// Estimate the section size according to the heap size and privilege level
		if( opened )
		{
			_MEMORY_BASIC_INFORMATION64 mbi;
			SIZE_T blockSize = VirtualQueryEx(ph, (LPCVOID) ((unsigned char*)base + offset), (_MEMORY_BASIC_INFORMATION*) &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

			if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
			{
				if( mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					// Good region
					return mbi.RegionSize;
				}
			}
			else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
			{
				_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
				if( mbi32->State == MEM_COMMIT && !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					return mbi32->RegionSize;
				}
			}
		}
		
		return 0; // Not valid
	}

	virtual void update_base( __int64 rva )
	{
		this->base = (void*) ((__int64)base + rva);
	}

	virtual __int64 get_address( )
	{
		return (__int64) this->base;
	}

	virtual bool read( long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read )
	{
		// Reads in memory by region. Skips noaccess, guard, and failures leaving the corresponding
		// parts of the output buffer untouched.
		*out_read = 0;

		SIZE_T num_read = 0;

		SIZE_T already_read = 0;

		if( opened )
		{
			while( already_read < size )
			{
				_MEMORY_BASIC_INFORMATION64 mbi;
				__int64 blockSize = VirtualQueryEx(ph, (LPCVOID) ((unsigned char*)base + (SIZE_T)offset + (SIZE_T)already_read), (_MEMORY_BASIC_INFORMATION*) &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));
				__int64 start_address = already_read + (__int64)base + offset;

				if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
				{
					if( mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
					{
						// Read in this whole or part of this region
						bool success;
						if( start_address + size - already_read >= mbi.BaseAddress + mbi.RegionSize )
						{
							// Read in the whole region
							success = ReadProcessMemory( ph,
																(LPCVOID) (start_address),
																(void*)((__int64) output + already_read),
																(SIZE_T)mbi.RegionSize,
																&num_read);
							already_read += mbi.RegionSize;
							*out_read += num_read;
						}
						else
						{
							// Read in the partial region
							success = ReadProcessMemory( ph,
																	(LPCVOID) (start_address),
																	(void*)((__int64) output + already_read),
																	(SIZE_T)(size - already_read),
																	&num_read);
								already_read += size - already_read;
								*out_read += num_read;
						}
					}
					else
					{
						// Guard or noaccess, skip this region
						already_read += mbi.RegionSize - (start_address - mbi.BaseAddress);
					}
				}
				else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
				{
					_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
					if( mbi32->State == MEM_COMMIT && !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
					{
						// Read in this whole or part of this region
						bool success;

						if( start_address + size - already_read >= (SIZE_T)mbi32->BaseAddress + mbi32->RegionSize )
						{
							// Read in the whole region
							success = ReadProcessMemory( ph,
																(LPCVOID) (start_address),
																(void*)((__int64) output + already_read),
																mbi32->RegionSize,
																&num_read);
							already_read += mbi32->RegionSize;
							*out_read += num_read;
						}
						else
						{
							// Read in the partial region
							success = ReadProcessMemory( ph,
																	(LPCVOID) (start_address),
																	(void*)((__int64) output + already_read),
																	(SIZE_T)(size - already_read),
																	&num_read);
								already_read += size - already_read;
								*out_read += num_read;
						}
					}
					else
					{
						// Guard or noaccess, skip this region
						already_read += mbi32->RegionSize - (start_address - mbi32->BaseAddress);
					}
				}
				else
				{
					// Failed to query MBI information, best we can do is skip 1 page?
					already_read += 0x1000;
				}
			}
		}

		if( *out_read != already_read )
			return false;
		return true;
	}

	~process_stream(void)
	{
		if( opened )
		{
			// Close the handle
			//CloseHandle( ph ); SHOULD NOT DO THIS. USED BY CREATOR.

			if( _long_name != NULL )
				delete[] _long_name;
			if( _short_name != NULL )
				delete[] _short_name;
		}
	}
};



