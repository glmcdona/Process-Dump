#include "StdAfx.h"
#include "dump_process.h"




dump_process::dump_process(DWORD pid, pe_hash_database* db, PD_OPTIONS* options, bool quieter)
{
	_options = options;
	_opened = false;
	_pid = pid;
	_ph = NULL;
	_process_name = NULL;
	_export_list_built = false;
	_term_hook = NULL;
	_loaded_is64 = false;
	_is64 = false;
	_address_main_module = 0;
	_quieter = quieter && !options->Verbose;

	// Load the clean hash database
	_db_clean = db;

	// Dump this specified PID into the current directory
	_ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pid);
	if (_ph == NULL)
	{
		// try opening with minimal permissions. This works for most actions (except terminate hooking)
		_ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
		if (_ph != NULL && _options->Verbose)
			fprintf(stderr, "WARNING: For PID 0x%x, we had to open handle with fewer permissions than expected. Dropped PROCESS_VM_WRITE and PROCESS_VM_OPERATION.\n", pid);
	}
	
	
	if( _ph != NULL )
	{
		// Try to load the main module name
		HANDLE hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if(hSnapshot != INVALID_HANDLE_VALUE )
		{
			_opened = true;

			// Load the main module name
			MODULEENTRY32 tmpModule;
			tmpModule.dwSize = sizeof(MODULEENTRY32);
			if( Module32First(hSnapshot, &tmpModule) )
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
				
				_address_main_module = (unsigned __int64) tmpModule.modBaseAddr;
			}

			CloseHandle(hSnapshot);
		}
		else
		{
			if (!_quieter)
			{
				if (GetLastError() == 299)
					fprintf(stderr, "ERROR: Unable to snapshot process PID 0x%x. This can be as a result of the process being a 64 bit process and this tool is running as a 32 bit process, or the process may have not finished being created or already closed.\n", pid);
				else
					PrintLastError(L"dump_process CreateToolhelp32Snapshot");
			}

			_process_name = new char[strlen("unknown")+1];
			strcpy( _process_name, "unknown" );
		}
	}
	else
	{
		if (!_quieter)
		{
			fprintf(stderr, "Failed to open process with PID 0x%x:\n", pid);
			PrintLastError(L"\tdump_process");
		}
	}
}

bool dump_process::get_process_name(char* process_name, SIZE_T byte_length)
{
	if (_process_name != NULL)
	{
		if (strlen(_process_name) < byte_length)
		{
			strcpy_s(process_name, byte_length, _process_name);
			return true;
		}
		if (byte_length > 0)
			_process_name[0] = 0;
	}
	
	return false;
}

bool dump_process::is64()
{
	if (!_loaded_is64)
	{
		// Look at the main module to determine if it is 64 or 32 bit
		module_list* modules = new module_list(); // empty
		pe_header* main_module = new pe_header(_ph, (void*) _address_main_module, modules, _options);
		main_module->process_pe_header();
		_is64 = main_module->is_64();
		_loaded_is64 = true;
		delete main_module;
		delete modules;
	}

	if (_loaded_is64)
		return _is64;
	
	// Failed. Assume 64 bit.
	fprintf(stderr, "ERROR: For PID 0x%x, was unable to look at main module to determine 32 or 64 bit mode.\n", _pid);
	return true;
}

MBI_BASIC_INFO dump_process::get_mbi_info(unsigned __int64 address)
{
	_MEMORY_BASIC_INFORMATION64 mbi;
	MBI_BASIC_INFO result{ 0 };

	// Load this heap information
	 __int64 blockSize = VirtualQueryEx(_ph, (LPCVOID)address, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

	if (blockSize == sizeof(_MEMORY_BASIC_INFORMATION64))
	{
		result.base = mbi.BaseAddress;
		result.end = mbi.BaseAddress + mbi.RegionSize;
		result.protect = mbi.Protect;
		result.valid = mbi.State != MEM_FREE && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
		result.executable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) > 0;
	}
	else if (blockSize == sizeof(_MEMORY_BASIC_INFORMATION32))
	{
		_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*)&mbi;

		result.base = mbi32->BaseAddress;
		result.end = (__int64)mbi32->BaseAddress + mbi32->RegionSize;
		result.protect = mbi32->Protect;
		result.valid = mbi32->State != MEM_FREE && !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD));
		result.executable = (mbi32->Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) > 0;
	}

	return result;
}


int dump_process::get_all_hashes(unordered_set<unsigned __int64>* output_hashes, unordered_set<unsigned __int64>* output_hashes_eps, unordered_set<unsigned __int64>* output_hashes_ep_shorts)
{
	// Adds all the modules in the process to the output array
	if( _ph != NULL )
	{
		if( !_options->DumpChunks || build_export_list() ) // Only build export list if getting hashes for code chunks
		{
			// First build a list of the modules
			module_list* modules = new module_list( _pid );
			
			// Set the max address of the target process
			unsigned __int64 maxAddress = 0;
			maxAddress = 0xffffffffffffffff; // Not a problem for 32bit targets
			
			// Walk the process heaps
			unsigned __int64 address = 0;
			
			// First loop to build a list of executable heaps for later use in locating loose executable heaps not associated with any modules
			set<unsigned __int64> executable_heaps;
			if( _options->DumpChunks )
			{
				while (address < maxAddress)
				{
					MBI_BASIC_INFO mbi_info = get_mbi_info(address);
					
					// Check if this is a loose executable heap
					if( mbi_info.base > 0 && mbi_info.end > 0 && mbi_info.valid && mbi_info.executable)
					{
						executable_heaps.insert(mbi_info.base);
					}
					
					if( (unsigned __int64)mbi_info.end + 1 <= address )
						break;
					address = mbi_info.end + 1;
				}
			}

			// Load all the PE files in the process
			address = 0;

			while (address < maxAddress)
			{
				// Load this heap information
				MBI_BASIC_INFO mbi_info = get_mbi_info(address);

				if (mbi_info.base > 0 && mbi_info.end > 0 && mbi_info.valid )
				{
					if( _options->Verbose )
						fprintf( stdout, "INFO: Scanning from region from 0x%llX to 0x%llX for MZ headers.\n", mbi_info.base, mbi_info.end );

					// This heap may have a PE file, check all page alignments for a "MZ".
					unsigned __int64 base = mbi_info.base - (mbi_info.base % PAGE_SIZE); // shouldn't be required.
					char output[2];
					SIZE_T out_read;
					int count = 0;
					while(mbi_info.base + 0x300 < mbi_info.end && count < 1000 ) // Skip the rest of the section if we have looped over 1000 pages.
					{
						if( ReadProcessMemory( _ph, (LPCVOID) ((unsigned char*) base), output, 2, &out_read) && out_read == 2 )
						{
							if( output[0] == 'M' && output[1] == 'Z' )
							{
								if( _options->Verbose )
									fprintf( stdout, "INFO: Found MZ header at %llX.\n", base );

								// Bingo, possible MZ file
								pe_header* header = new pe_header( _pid, (void*) base, modules, _options );

								header->process_pe_header();
								header->process_sections();

								if( header->somewhat_parsed() )
								{
									// Exclude all executable regions from this PE module
									unsigned __int64 end_address = header->get_virtual_size() + base;
									for (set<unsigned __int64>::iterator it=executable_heaps.begin(); it!=executable_heaps.end(); )
									{
										if( *it <= end_address && *it >= base )
										{
											// We've accounted for this executable heap, remove it from the loose heap list
											it = executable_heaps.erase(it);
										}
										else
										{
											it++;
										}
									}

									// Check hash
									unsigned __int64 hash = header->get_hash();
									if( hash != 0 && !_db_clean->contains(hash) && output_hashes->count( hash ) == 0 )
									{
										// Add this to the output hash array
										output_hashes->insert( hash );
									}

									if (output_hashes_ep_shorts != NULL)
									{
										// Also get the entrypoint hash
										hash = header->get_hash_ep_short();

										if (hash != 0)
										{
											if( output_hashes_ep_shorts->count(hash) == 0 )
											{
												// Add this to the output hash array
												output_hashes_ep_shorts->insert(hash);
											}
										}

										if (output_hashes_eps != NULL)
										{
											// Also get the entrypoint hash
											hash = header->get_hash_ep();

											if (hash != 0 && output_hashes_eps->count(hash) == 0)
											{
												// Add this to the output hash array
												output_hashes_eps->insert(hash);
											}
										}
									}
								}
								delete header;
							}
						}

						base += PAGE_SIZE;
						count++;
					}
				}

				if ((unsigned __int64)mbi_info.end + 1 <= address)
					break;
				address = mbi_info.end + 1;
			}

			
			if( _options->DumpChunks )
			{
				// One last loop to add the hashes from all stray executable heaps
				if( _options->Verbose )
					fprintf( stdout, "INFO: Looking at unattached executable heaps...\n" );
				
				int count_new_header_hashes = 0;
				for (set<unsigned __int64>::iterator it=executable_heaps.begin(); it!=executable_heaps.end(); it++)
				{
					// Unattached executable page. First check hash of crc32 of first 2kb of memory since import reconstruction hashing is
					// very expensive.
					unsigned __int64 chunk_header_hash = this->hash_codechunk_header(*it);

					if( _options->Verbose )
						fprintf( stdout, "INFO: Unattached heap start hash 0x%llX\n", chunk_header_hash );

					if( chunk_header_hash != 0 && !_db_clean->contains(chunk_header_hash) && output_hashes->count( chunk_header_hash ) == 0 )
					{
						if( _options->Verbose )
							fprintf( stdout, "INFO: Unattached heap start hash is new.\n" );
						
						if( count_new_header_hashes++ > CODECHUNK_NEW_HASH_LIMIT )
						{
							if( _options->Verbose )
								fprintf( stdout, "INFO: Too many unique loose code chunks. Stopped processing more chunks.\n" );
							break; // Too many new code chunks
						}

						// Add this header hash
						output_hashes->insert( chunk_header_hash );

						// Calculate the generic import reference hash as well
						pe_header header( _pid, (void*) *it, modules, _options );
						header.build_pe_header( 0x1000, true, 1 ); // 64bit, only build it with the 1 executable section for performance reasons
						header.process_sections();

						// Get the import attributes of this header
						IMPORT_SUMMARY import_summary = header.get_imports_information(&this->_export_list);

						// Check hash
						if( import_summary.HASH_GENERIC != 0 && !_db_clean->contains(import_summary.HASH_GENERIC) && output_hashes->count( import_summary.HASH_GENERIC ) == 0 )
						{
							if( _options->Verbose )
								fprintf( stdout, "INFO: Adding hash from unattached heap at 0x%llX to process hash list: Hash=0x%llX\n", *it, import_summary.HASH_GENERIC );
							
							output_hashes->insert( import_summary.HASH_GENERIC );
						}
					}
				}
				if( _options->Verbose )
					fprintf( stdout, "INFO: Done looking at unattached executable heaps...\n" );
			}

			delete modules;
		}
	}
	else if( _options->Verbose )
		fprintf( stdout, "INFO: Null process handle %s.\n", this->_process_name );
	
	return false;
}

unsigned __int64 dump_process::hash_codechunk_header(__int64 base)
{
	char header_buffer[CODECHUNK_HEADER_HASH_SIZE];
	SIZE_T num_read = 0;
	
	BOOL success = ReadProcessMemory( _ph,
																(LPCVOID) (base),
																(void*)(header_buffer),
																CODECHUNK_HEADER_HASH_SIZE,
																&num_read);

	if( ( success || GetLastError() == ERROR_PARTIAL_COPY ) && num_read > 8 && num_read <= CODECHUNK_HEADER_HASH_SIZE )
	{
		// Hash the content
		return (unsigned __int64) crc32buf(header_buffer, num_read);
	}
	
	// Default bad hash
	return 0;
}

bool dump_process::build_export_list()
{
	// Walk through each module, building the export list for this process. This will be used for import reconstruction
	// Returns: True if there are any modules to dump, False if there is nothing to dump.

	if( !_export_list_built )
	{
		if( !_quieter )
			printf( "... building import reconstruction table ...\n" );
		
		if (_ph != NULL)
		{
			// First build a list of the modules
			module_list* modules = new module_list(_pid);

			// Loop through each of these modules, grabbing their exports
			for (unordered_map<unsigned __int64, module*>::const_iterator item = modules->_modules.begin(); item != modules->_modules.end(); ++item)
			{
				pe_header header(_pid, (void*) item->first, modules, _options);
				if (header.process_pe_header() && header.process_sections() && header.process_export_directory())
				{
					// Load it's exports
					this->_export_list.add_exports(header.get_exports());
				}
			}

			delete modules;
		}
		_export_list_built = true;
	}

	return true;
}

bool dump_process::build_export_list(export_list* result, char* library, module_list* modules)
{
	// Walk through each module, building the export list for this process. This will be used for import reconstruction
	// Returns: True if there are any modules to dump, False if there is nothing to dump.
	if (_ph != NULL)
	{
		// Loop through each of these modules, grabbing their exports
		for (unordered_map<unsigned __int64, module*>::const_iterator item = modules->_modules.begin(); item != modules->_modules.end(); ++item)
		{
			if (_strcmpi(item->second->short_name, library) == 0)
			{
				pe_header header(_pid, (void*)item->first, modules, _options);
				if (header.process_pe_header() && header.process_sections() && header.process_export_directory())
				{
					// Load its exports
					result->add_exports(header.get_exports());
				}
			}
		}
	}

	return true;
}

void dump_process::dump_header(pe_header* header, __int64 base, DWORD pid)
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
					if( _options->Verbose )
							printf(" preparing disk image for '%s' at %llX\n", header->get_name(), (__int64) base);
					if( header->process_disk_image(&this->_export_list, this->_db_clean ) )
					{
						// Build the name that we will dump this image as
						char* extension = ( header->is_exe() ? "exe" :
														( header->is_dll() ? "dll" : 
														( header->is_sys() ? "sys" : "bin" ) ) );
						int length = MAX_PATH + FILENAME_MAX + 1;
						char* filename = new char[length];
						if( _options->output_path != NULL && strlen(_options->output_path) > 0 )
							sprintf(filename, "%s\\%s_PID%x_%s_%llX_%s.%s", _options->output_path, _process_name, pid, header->get_name(), (__int64)  base, (header->is_64() ? "x64": "x86"), extension );
						else
							sprintf(filename, "%s_PID%x_%s_%llX_%s.%s", _process_name, pid, header->get_name(), (__int64)  base, (header->is_64() ? "x64": "x86"), extension );
						
						// Dump the module
						printf(" dumping '%s' at %llX to file '%s'\n", extension, (__int64) base, filename);
						header->write_image(filename);
						
						delete[] filename;
					}
					else
					{
						if( _options->Verbose )
							printf("Failed to process disk image for module at %llX\n", base);
					}
				}
				else
				{
					if( _options->Verbose )
						printf("Null hash or the has is already in the clean hash database at %llX\n", base);
				}

			}
			else
			{
				if( _options->Verbose )
					printf("Failed to process import directory for module at %llX\n", base);
			}
		}
		else
		{
			if( _options->Verbose )
				printf("Module was not somehwat parsed for module at %llX\n", base);
		}
	}
	else
	{
		if( _options->Verbose )
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
		if( !_options->ImportRec || build_export_list() )
		{
			module_list* modules = new module_list( _pid );
			pe_header* header = new pe_header( _pid, (void*) base, modules, _options );
			
			if( _options->ForceGenHeader || !header->process_pe_header() )
			{
				if( _options->Verbose )
					printf( "Generating 32-bit PE header for module at %llX.\n", base );
				
				// Build the pe header as 32 and 64 bit since it could be either
				header->build_pe_header(0x1000ffff, true );
				dump_header(header, base, _pid);
				delete header;

				if( _options->Verbose )
					printf( "Generating 64-bit PE header for module at %llX.\n", base );
				header = new pe_header( _pid, (void*) base, modules, _options );
				header->build_pe_header(0x1000ffff, false ); 
				dump_header(header, base, _pid);
			}
			else
			{
				if( _options->Verbose )
					printf( "Using existing PE header for module at %llX.\n", base );
				dump_header(header, base, _pid);
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


bool dump_process::monitor_close_start()
{
	if (!_opened || _address_main_module == 0)
		return false; // Not attached well to this process

	if (_term_hook == NULL)
	{
		// Add a hook on for when the process terminates so that we can dump it.
		if( _options->Verbose )
			printf("Hooking process terminate for process %s...\n", this->_process_name);
		_term_hook = new terminate_monitor_hook(_ph, _pid, this->is64(), this->_options );

		// Load the exports needed for the hooks
		module_list* modules = new module_list(_pid);
		export_list* exports = new export_list();
		build_export_list(exports, "kernel32.dll", modules);
		build_export_list(exports, "ntdll.dll", modules);
		bool result = _term_hook->hook_terminate(exports);

		// Cleanup
		delete exports;
		delete modules;

		return result;
	}

	return true; // Already started
}

bool dump_process::monitor_close_is_waiting()
{
	if (_term_hook != NULL)
	{
		return _term_hook->is_terminate_waiting();
	}

	return false; // not hooked
}

bool dump_process::monitor_close_stop()
{
	if (_term_hook != NULL)
	{
		delete _term_hook;
		return true;
	}
	return true; // not hooked
}


bool dump_process::monitor_close_dump_and_resume()
{
	if (_term_hook != NULL)
	{
		if (_term_hook->is_terminate_waiting())
		{
			// Dump the process
			dump_all();

			// Resume it so that it closes normally
			_term_hook->resume_terminate();

			return true;
		}

		return false; // was not waiting
	}
	return false; // not hooked
}

void dump_process::dump_all()
{
	// Walk through the pages while dumping all MZ files that do not match our good hash database.
	printf( "dumping process %s with pid 0x%x...\n", this->_process_name, this->_pid );
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
			unsigned __int64 address = 0;
			
			// First loop to build a list of executable heaps for later use in locating loose executable heaps not associated with any modules
			set<unsigned __int64> executable_heaps;
			if (_options->DumpChunks)
			{
				while (address < maxAddress)
				{
					MBI_BASIC_INFO mbi_info = get_mbi_info(address);

					// Check if this is a loose executable heap (this check causes it to fail.)
					if (mbi_info.base > 0 && mbi_info.end > 0 && mbi_info.valid && mbi_info.executable)
					{
						executable_heaps.insert(mbi_info.base);
					}

					if ((unsigned __int64)mbi_info.end + 1 <= address)
						break;
					address = mbi_info.end + 1;
				}
			}

			// Load all the PE files in the process
			address = 0;
			while (address < maxAddress)
			{
				MBI_BASIC_INFO mbi_info = get_mbi_info(address);

				// Check if this is a loose executable heap
				if ( mbi_info.base > 0 && mbi_info.end > 0 && mbi_info.valid )
				{
					if (_options->Verbose)
						fprintf(stdout, "INFO: Scanning from region from 0x%llX to 0x%llX for MZ headers.\n", mbi_info.base, mbi_info.end);


					// This heap may have a PE file, check all page alignments for a "MZ".
					unsigned __int64 base = mbi_info.base - (mbi_info.base % PAGE_SIZE); // shouldn't be required.
					char output[2];
					SIZE_T out_read;
					int count = 0;
					while (base + 0x300 < (unsigned __int64)mbi_info.end && count < 1000) // Skip the rest of the section if we have looped over 1000 pages.
					{
						if (ReadProcessMemory(_ph, (LPCVOID)((unsigned char*)base), output, 2, &out_read) && out_read == 2)
						{
							if (output[0] == 'M' && output[1] == 'Z')
							{
								// Bingo, possible MZ file
								pe_header* header = new pe_header(_pid, (void*)base, modules, _options);

								// Use the existing PE header for the dumping
								if (header->process_pe_header())
								{
									if (header->process_sections() && header->somewhat_parsed() && header->process_import_directory())
									{
										// Exclude all executable regions from this PE module
										unsigned __int64 end_address = header->get_virtual_size() + base;
										for (set<unsigned __int64>::iterator it = executable_heaps.begin(); it != executable_heaps.end(); )
										{
											if ( *it <= end_address && *it >= base)
											{
												// We've accounted for this executable heap, remove it from the loose heap list
												it = executable_heaps.erase(it);
											}
											else
											{
												it++;
											}
										}

										// Check hash
										unsigned __int64 hash = header->get_hash();
										if (hash != 0 && !_db_clean->contains(hash))
										{
											if (_options->ForceGenHeader)
											{
												// Use the existing PE header only to get the hash, then generate a PE header for the dumping.
												fprintf(stdout, "Dumping a module but ignoring existing PE Header for module at 0x%llX.\n", base);
												pe_header* header_dump = new pe_header(_pid, (void*)base, modules, _options);
												header_dump->build_pe_header(0x1000, true); // 64bit
												dump_header(header_dump, base, _pid);
												delete header_dump;

												header_dump = new pe_header(_pid, (void*)base, modules, _options);
												header_dump->build_pe_header(0x1000, false); // 32bit
												dump_header(header_dump, base, _pid);
												delete header_dump;
											}
											else if (header->process_disk_image(&this->_export_list, this->_db_clean))
											{
												// Build the name that we will dump this image as
												char* extension = (header->is_exe() ? "exe" :
													(header->is_dll() ? "dll" :
														(header->is_sys() ? "sys" : "bin")));
												int length = MAX_PATH + FILENAME_MAX + 1;
												char* filename = new char[length];
												if (_options->output_path != NULL && strlen(_options->output_path) > 0)
													sprintf(filename, "%s\\%s_PID%x_%s_%llX_%s.%s", _options->output_path, _process_name, _pid, header->get_name(), (__int64)base, (header->is_64() ? "x64" : "x86"), extension);
												else
													sprintf(filename, "%s_PID%x_%s_%llX_%s.%s", _process_name, _pid, header->get_name(), (__int64)base, (header->is_64() ? "x64" : "x86"), extension);

												// Dump the module
												printf(" dumping '%s' at %llX to file '%s'\n", extension, (__int64)base, filename);
												header->write_image(filename);

												delete[] filename;
											}
										}
									}
								}

								// Cleanup
								delete header;
							}
						}
						else
						{

						}

						base += PAGE_SIZE;
						count++;
					}
				}

				if ((unsigned __int64)mbi_info.end + 1 <= address)
					break;
				address = mbi_info.end + 1;
			}

			if( _options->DumpChunks )
			{
				// One last loop to add the hashes from all stray executable heaps
				if( _options->Verbose )
					fprintf( stdout, "INFO: Looking at unattached executable heaps...\n" );

				int count_new_header_hashes = 0;
				for (set<unsigned __int64>::iterator it=executable_heaps.begin(); it!=executable_heaps.end(); it++)
				{
					// Unattached executable page. First check hash of crc32 of first 2kb of memory since import reconstruction hashing is
					// very expensive.
					unsigned __int64 chunk_header_hash = this->hash_codechunk_header(*it);

					if( _options->Verbose )
						fprintf( stdout, "INFO: Unattached heap start hash 0x%llX\n", chunk_header_hash );

					if( chunk_header_hash != 0 && !_db_clean->contains(chunk_header_hash) )
					{
						if( _options->Verbose )
							fprintf( stdout, "INFO: Unattached heap start hash is new.\n" );
						
						if( count_new_header_hashes++ > CODECHUNK_NEW_HASH_LIMIT )
						{
							if( _options->Verbose )
								fprintf( stdout, "INFO: Too many unique loose code chunks. Stopped processing more chunks.\n" );
							break; // Too many new code chunks
						}
						
						// Calculate the generic import reference hash as well
						pe_header* header = new pe_header( _pid, (void*) *it, modules, _options );
						header->build_pe_header( 0x1000, true, 1 ); // 64bit, only build it with the 1 executable section for performance reasons
						header->process_sections();

						// Get the import attributes of this header
						IMPORT_SUMMARY import_summary = header->get_imports_information(&this->_export_list);

						// Check hash
						if( import_summary.HASH_GENERIC != 0 && !_db_clean->contains(import_summary.HASH_GENERIC) )
						{
							if( _options->Verbose )
								fprintf( stdout, "INFO: Unattached executable heap at 0x%llX found with %zu imports matched.\n", *it, import_summary.COUNT_UNIQUE_IMPORT_ADDRESSES );
							
							if( header->somewhat_parsed() && import_summary.COUNT_UNIQUE_IMPORT_ADDRESSES >= 2 ) // Require at least 5 imports for dumping
							{
								fprintf( stdout, "Dumping unattached executable code chunk from 0x%llX.\n", *it );
								pe_header* header_dump = new pe_header( _pid, (void*) *it, modules, _options );
								header_dump->build_pe_header( 0x1000, true ); // 64bit
								header_dump->set_name("codechunk");
								dump_header(header_dump, *it, _pid);
								delete header_dump;
								
								header_dump = new pe_header( _pid, (void*) *it, modules, _options );
								header_dump->build_pe_header( 0x1000, false ); // 32bit
								header_dump->set_name("codechunk");
								dump_header(header_dump, *it, _pid);
								delete header_dump;
							}
						}
						delete header;
					}
				}
				if( _options->Verbose )
					fprintf( stdout, "INFO: Done looking at unattached executable heaps...\n" );
			}

			delete modules;
		}
	}
}



dump_process::~dump_process(void)
{
	if (_term_hook != NULL)
		delete _term_hook;
	if (_process_name != NULL)
		delete[] _process_name;
	if( _ph != NULL )
		CloseHandle( _ph );
}
