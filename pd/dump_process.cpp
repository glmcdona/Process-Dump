#include "StdAfx.h"
#include "dump_process.h"


dump_process::dump_process(DWORD pid, pe_hash_database* db, PD_OPTIONS options)
{
	_options = options;
	_opened = false;
	_pid = pid;
	_hSnapshot = NULL;
	_ph = NULL;
	_process_name = NULL;

	// Load the clean hash database
	_db_clean = db;

	// Dump this specified PID into the current directory
	_ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
	
	
	if( _ph != NULL )
	{
		_opened = true;

		// Try to load the main module name
		_hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if( _hSnapshot != INVALID_HANDLE_VALUE )
		{
			// Load the main module name
			MODULEENTRY32 tmpModule;
			tmpModule.dwSize = sizeof(MODULEENTRY32);
			if( Module32First(_hSnapshot, &tmpModule) )
			{
				_process_name = new char[wcslen(tmpModule.szModule) + 1];
				sprintf( _process_name, "%S", tmpModule.szModule );

				// Replace all '.'s in filename with underscores
				int i = 0;
				while( _process_name[i] != 0 )
				{
					if( _process_name[i] == '.' )
						_process_name[i] = '_';
					i++;
				}
			}
		}
		else
		{
			if( GetLastError() == 299 )
				fprintf(stderr, "ERROR: Unable to snapshot process PID 0x%x since it is a 64 bit process and this tool is running as a 32 bit process.\n", pid);
			else
				PrintLastError(L"dump_process CreateToolhelp32Snapshot");

			_process_name = new char[strlen("unknown")+1];
			strcpy( _process_name, "unknown" );
		}
	}
	else
	{
		fprintf(stderr, "Failed to open process with PID 0x%x:\n", pid );
		PrintLastError(L"\tdump_process");
	}
}


int dump_process::get_all_hashes(DynArray<unsigned __int64>* output_hashes)
{
	// Adds all the modules in the process to the output array
	if( _ph != NULL )
	{

		// First build a list of the modules
		module_list* modules = new module_list( _pid );

		// Set the max address of the target process
		unsigned __int64 maxAddress = 0;
		maxAddress = 0xffffffffffffffff; // Not a problem for 32bit targets

		// Walk the process heaps
		__int64 address = 0;
		_MEMORY_BASIC_INFORMATION64 mbi;
		
		while (address < maxAddress)
		{
			// Load this heap information
			__int64 blockSize = 0;
			__int64 newAddress = -1;
			blockSize = VirtualQueryEx(_ph, (LPCVOID) address, (PMEMORY_BASIC_INFORMATION)  &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

			__int64 base = 0;
			__int64 end = 0;

			if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
			{
				newAddress = (__int64)mbi.BaseAddress + (__int64)mbi.RegionSize + 1;

				if( !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					base = mbi.BaseAddress;
					end = mbi.BaseAddress + mbi.RegionSize;
				}
			}
			else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
			{
				_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
				newAddress = (__int64)mbi32->BaseAddress + (__int64)mbi32->RegionSize + 1;

				if( !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					base = mbi32->BaseAddress;
					end = mbi32->BaseAddress + mbi32->RegionSize;
				}
			}

			if( base > 0 && end > 0 )
			{
				if( _options.Verbose )
					fprintf( stdout, "INFO: Scanning from region from 0x%llX to 0x%llX for MZ headers.\r\n", base, end );

				// This heap may have a PE file, check all page alignments for a "MZ".
				base = base - (base % PAGE_SIZE); // shouldn't be required.
				char output[2];
				SIZE_T out_read;
				int count = 0;
				while( base + 0x300 < end && count < 1000 ) // Skip the rest of the section if we have looped over 1000 pages.
				{
					if( ReadProcessMemory( _ph, (LPCVOID) ((unsigned char*)base), output, 2, &out_read) && out_read == 2 )
					{
						if( output[0] == 'M' && output[1] == 'Z' )
						{
							if( _options.Verbose )
								fprintf( stdout, "INFO: Found MZ header at %llX.\r\n", base );

							// Bingo, possible MZ file
							pe_header* header = new pe_header( _pid, (void*) base, modules, _options );

							header->process_pe_header();
							header->process_sections();

							if( header->somewhat_parsed() )
							{
								// Check hash
								unsigned __int64 hash = header->get_hash();
								if( hash != 0 && !_db_clean->contains(hash) )
								{
									// Add this to the output dyn array
									output_hashes->Add( header->get_hash() );
								}
							}
							delete header;
						}
					}

					base += PAGE_SIZE;
					count++;
				}
			}

			if( newAddress <= address )
				break;
			address = newAddress;
		}

		delete modules;
	}
	else if( _options.Verbose )
		fprintf( stdout, "INFO: Null process handle %s.\r\n", this->_process_name );

	return false;
}

bool dump_process::build_export_list()
{
	// Walk through each module, building the export list for this process. This will be used for import reconstruction
	// Returns: True if there are any modules to dump, False if there is nothing to dump.
	printf( "... building import reconstruction table ...\n" );

	if( _ph != NULL )
	{
		// First build a list of the modules
		module_list* modules = new module_list( _pid );

		// Set the max address of the target process
		unsigned __int64 maxAddress = 0;
		maxAddress = 0xffffffffffffffff; // Not a problem for 32bit targets

		// Walk the process heaps
		__int64 address = 0;
		_MEMORY_BASIC_INFORMATION64 mbi;
		
		while (address < maxAddress)
		{
			// Load this heap information
			__int64 blockSize = 0;
			__int64 newAddress = -1;
			blockSize = VirtualQueryEx(_ph, (LPCVOID) address, (PMEMORY_BASIC_INFORMATION)  &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

			__int64 base = 0;
			__int64 end = 0;

			if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
			{
				newAddress = (__int64)mbi.BaseAddress + (__int64)mbi.RegionSize + 1;

				if( !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					base = mbi.BaseAddress;
					end = mbi.BaseAddress + mbi.RegionSize;
				}
			}
			else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
			{
				_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
				newAddress = (__int64)mbi32->BaseAddress + (__int64)mbi32->RegionSize + 1;

				if( !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
				{
					base = mbi32->BaseAddress;
					end = mbi32->BaseAddress + mbi32->RegionSize;
				}
			}

			if( base > 0 && end > 0 )
			{
				if( _options.Verbose )
					fprintf( stdout, "INFO: Scanning from region from 0x%llX to 0x%llX for MZ headers.\r\n", base, end );

				// This heap may have a PE file, check all page alignments for a "MZ".
				base = base - (base % PAGE_SIZE); // shouldn't be required.
				char output[2];
				SIZE_T out_read;
				int count = 0;
				while( base + 0x300 < end && count < 1000 ) // Skip the rest of the section if we have looped over 1000 pages.
				{
					if( ReadProcessMemory( _ph, (LPCVOID) ((unsigned char*)base), output, 2, &out_read) && out_read == 2 )
					{
						if( output[0] == 'M' && output[1] == 'Z' )
						{
							// Bingo, possible MZ file
							pe_header* header = new pe_header( _pid, (void*) base, modules, _options );
							if( header->process_pe_header() && header->process_sections() && header->process_export_directory() )
							{
								// Load it's exports
								this->_export_list.add_exports(header->get_exports());
							}

							// Cleanup
							delete header;
						}
					}

					base += PAGE_SIZE;
					count++;
				}
			}

			if( newAddress <= address )
				break;
			address = newAddress;
		}

		delete modules;
	}

	return true;
}

void dump_process::dump_header(pe_header* header, __int64 base)
{
	if( header->process_sections() )
	{
		if( header->somewhat_parsed() )
		{
			if( header->process_import_directory() )
			{
				// Check hash
				unsigned __int64 hash = header->get_hash();
				if( hash != 0 && !_db_clean->contains(hash) )
				{
					if( _options.Verbose )
							printf(" preparing disk image for '%s' at %llX\n", header->get_name(), (__int64) base);
					if( header->process_disk_image(&this->_export_list) )
					{
						// Build the name that we will dump this image as
						char* extension = ( header->is_exe() ? "exe" :
														( header->is_dll() ? "dll" : 
														( header->is_sys() ? "sys" : "bin" ) ) );
						int length = strlen("%s_%s_%s_%llX.%s") + 0x8 + strlen(_process_name) + 1 + strlen(header->get_name()) + 0x16 + strlen(extension) + 1;
						char* filename = new char[length];
						sprintf(filename, "%s_%s_%s_%llX.%s", _process_name, (header->is_64() ? "x64": "x86") , header->get_name(), (__int64)  base, extension );
						
						// Dump the module
						printf(" dumping '%s' at %llX to file '%s'\n", extension, (__int64) base, filename);
						header->write_image(filename);
						
						delete[] filename;
					}
					else
					{
						if( _options.Verbose )
							printf("Failed to process disk image for module at %llX\n", base);
					}
				}
				else
				{
					if( _options.Verbose )
						printf("Null hash or the has is already in the clean hash database at %llX\n", base);
				}

			}
			else
			{
				if( _options.Verbose )
					printf("Failed to process import directory for module at %llX\n", base);
			}
		}
		else
		{
			if( _options.Verbose )
				printf("Module was not somehwat parsed for module at %llX\n", base);
		}
	}
	else
	{
		if( _options.Verbose )
			printf("Failed to process sections for module at %llX\n", base);
	}
}

void dump_process::dump_region(__int64 base)
{
	// Walk through the pages while dumping all MZ files that do not match our good hash database.
	printf( "\ndumping starting at %llX from process %s with pid 0x%x...\n", (__int64) base, this->_process_name, this->_pid );
	if( _ph != NULL )
	{
		// First build the export list for this process
		if( !_options.ImportRec || build_export_list() )
		{
			module_list* modules = new module_list( _pid );
			pe_header* header = new pe_header( _pid, (void*) base, modules, _options );
			
			if( _options.ForceGenHeader || !header->process_pe_header() )
			{
				if( _options.Verbose )
					printf( "Generating 32-bit PE header for module at %llX.\n", base );
				
				// Build the pe header as 32 and 64 bit since it could be either
				header->build_pe_header(0x1000ffff, true );
				dump_header(header, base);
				delete header;

				if( _options.Verbose )
					printf( "Generating 64-bit PE header for module at %llX.\n", base );
				header = new pe_header( _pid, (void*) base, modules, _options );
				header->build_pe_header(0x1000ffff, false ); 
				dump_header(header, base);
			}
			else
			{
				if( _options.Verbose )
					printf( "Using existing PE header for module at %llX.\n", base );
				dump_header(header, base);
			}



			delete modules;
			delete header;
		}
		else
		{
			printf("Failed to build export list.\n");
		}

	}
}


void dump_process::dump_all()
{
	// Walk through the pages while dumping all MZ files that do not match our good hash database.
	printf( "\ndumping process %s with pid 0x%x...\n", this->_process_name, this->_pid );
	if( _ph != NULL )
	{
		// First build the export list for this process
		if( build_export_list() )
		{
			// First build a list of the modules
			module_list* modules = new module_list( _pid );

			// Set the max address of the target process
			unsigned __int64 maxAddress = 0;
			maxAddress = 0xffffffffffffffff; // Not a problem for 32bit targets

			// Walk the process heaps
			__int64 address = 0;
			_MEMORY_BASIC_INFORMATION64 mbi;
			
			while (address < maxAddress)
			{
				// Load this heap information
				__int64 blockSize = 0;
				__int64 newAddress = -1;
				blockSize = VirtualQueryEx(_ph, (LPCVOID) address, (PMEMORY_BASIC_INFORMATION)  &mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

				__int64 base = 0;
				__int64 end = 0;

				if( blockSize == sizeof(_MEMORY_BASIC_INFORMATION64) )
				{
					newAddress = (__int64)mbi.BaseAddress + (__int64)mbi.RegionSize + 1;

					if( !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
					{
						base = mbi.BaseAddress;
						end = mbi.BaseAddress + mbi.RegionSize;
					}
				}
				else if(  blockSize == sizeof(_MEMORY_BASIC_INFORMATION32) )
				{
					_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*) &mbi;
					newAddress = (__int64)mbi32->BaseAddress + (__int64)mbi32->RegionSize + 1;

					if( !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD)) )
					{
						base = mbi32->BaseAddress;
						end = mbi32->BaseAddress + mbi32->RegionSize;
					}
				}

				if( base > 0 && end > 0 )
				{
					if( _options.Verbose )
						fprintf( stdout, "INFO: Scanning from region from 0x%llX to 0x%llX for MZ headers.\r\n", base, end );


					// This heap may have a PE file, check all page alignments for a "MZ".
					base = base - (base % PAGE_SIZE); // shouldn't be required.
					char output[2];
					SIZE_T out_read;
					int count = 0;
					while( base + 0x300 < end && count < 1000 ) // Skip the rest of the section if we have looped over 1000 pages.
					{
						if( ReadProcessMemory( _ph, (LPCVOID) ((unsigned char*)base), output, 2, &out_read) && out_read == 2 )
						{
							if( output[0] == 'M' && output[1] == 'Z' )
							{
								// Bingo, possible MZ file
								pe_header* header = new pe_header( _pid, (void*) base, modules, _options );
								if( header->process_pe_header() )
								{
									if( header->process_sections() && header->somewhat_parsed() && header->process_import_directory() )
									{
										// Check hash
										unsigned __int64 hash = header->get_hash();
										if( hash != 0 && !_db_clean->contains(hash) && header->process_disk_image(&this->_export_list) )
										{
											// Build the name that we will dump this image as
											char* extension = ( header->is_exe() ? "exe" :
																			( header->is_dll() ? "dll" : 
																			( header->is_sys() ? "sys" : "bin" ) ) );
											int length = strlen("%s_%s_%llX.%s") + 0x8 + strlen(_process_name) + strlen(header->get_name()) + 0x16 + strlen(extension) + 1;
											char* filename = new char[length];
											sprintf(filename, "%s_%s_%llX.%s", _process_name, header->get_name(), (__int64) base, extension );

											// Dump the module
											printf(" dumping '%s' at %llX to file '%s'\n", extension, (__int64)  base, filename);
											header->write_image(filename);

											delete[] filename;
										}
									}
								}

								// Cleanup
								delete header;
							}
						}

						base += PAGE_SIZE;
						count++;
					}
				}

				if( newAddress <= address )
					break;
				address = newAddress;
			}

			delete modules;
		}
	}
}



dump_process::~dump_process(void)
{
	if( _ph != NULL )
		CloseHandle( _ph );
	if( _hSnapshot != NULL )
		CloseHandle( _hSnapshot );
	if( _process_name != NULL )
		delete[] _process_name;
}
