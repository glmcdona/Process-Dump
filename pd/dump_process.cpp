#include "StdAfx.h"
#include "dump_process.h"
#include "dump_path.h"
#include "module_work.h"




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
	_ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE, false, pid);
	if (_ph == NULL)
	{
		// try opening with minimal permissions. This works for most actions (except terminate hooking)
		_ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
		if (_ph != NULL && _options->Verbose)
			fprintf(stderr, "WARNING: For PID 0x%x, we had to open handle with fewer permissions than expected. Dropped PROCESS_VM_WRITE, PROCESS_VM_OPERATION and PROCESS_DUP_HANDLE.\n", pid);
	}
	
	
	if( _ph != NULL )
	{
		// Try to load the main module name
		HANDLE hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if(hSnapshot != INVALID_HANDLE_VALUE )
		{
			_opened = true;

			// Load the main module name
			MODULEENTRY32 tmpModule = {};
			tmpModule.dwSize = sizeof(MODULEENTRY32);
			if( Module32First(hSnapshot, &tmpModule) )
			{
				std::string converted;
				if (module_names::to_ansi(tmpModule.szModule, _countof(tmpModule.szModule), converted, "process name"))
				{
					_process_name = new char[converted.size() + 1];
					memcpy(_process_name, converted.c_str(), converted.size() + 1);

					// Preserve the existing process-label convention.
					for (SIZE_T i = 0; i < converted.size(); ++i)
						if (_process_name[i] == '.')
							_process_name[i] = '_';
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

		}
		if (_process_name == NULL)
		{
			_process_name = new char[sizeof("unknown")];
			memcpy(_process_name, "unknown", sizeof("unknown"));
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
	if (process_name == NULL || byte_length == 0)
		return false;
	process_name[0] = 0;
	if (_process_name != NULL)
	{
		if (strlen(_process_name) < byte_length)
		{
			strcpy_s(process_name, byte_length, _process_name);
			return true;
		}
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
	MBI_BASIC_INFO result;
	result.base = 0;
	result.end = 0;
	result.protect = 0;
	result.valid = false;
	result.executable = false;

	// Load this heap information
	 __int64 blockSize = VirtualQueryEx(_ph, (LPCVOID)address, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(_MEMORY_BASIC_INFORMATION64));

	if (blockSize == sizeof(_MEMORY_BASIC_INFORMATION64))
	{
		result.base = mbi.BaseAddress;
		result.end = mbi.BaseAddress + mbi.RegionSize;
		result.protect = mbi.Protect;
		result.valid = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
		result.executable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) > 0;
	}
	else if (blockSize == sizeof(_MEMORY_BASIC_INFORMATION32))
	{
		_MEMORY_BASIC_INFORMATION32* mbi32 = (_MEMORY_BASIC_INFORMATION32*)&mbi;

		result.base = mbi32->BaseAddress;
		result.end = mbi32->BaseAddress + mbi32->RegionSize;
		result.protect = mbi32->Protect;
		result.valid = mbi32->State == MEM_COMMIT && !(mbi32->Protect & (PAGE_NOACCESS | PAGE_GUARD));
		result.executable = (mbi32->Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) > 0;
	}

	return result;
}


std::vector<unsigned __int64> dump_process::scan_regions(set<unsigned __int64>& executable_heaps)
{
	std::vector<unsigned __int64> bases;
	for (unsigned __int64 address = 0;;)
	{
		const auto region = get_mbi_info(address);
		if (region.base != 0 && region.end > region.base && region.valid)
		{
			if (_options->DumpChunks && region.executable)
				executable_heaps.insert(region.base);
			if (_options->Verbose)
				printf("INFO: Scanning region 0x%llX to 0x%llX for MZ headers.\n", region.base, region.end);
			unsigned __int64 base = region.base - region.base % PAGE_SIZE;
			for (int page = 0; page < 1000 && base < region.end && region.end - base > 0x300; ++page, base += PAGE_SIZE)
			{
				WORD signature = 0;
				SIZE_T read = 0;
				if (ReadProcessMemory(_ph, reinterpret_cast<void*>(base), &signature, sizeof(signature), &read) &&
					read == sizeof(signature) && signature == IMAGE_DOS_SIGNATURE)
					bases.push_back(base);
			}
		}
		if (region.end <= address)
			break;
		address = region.end;
	}
	return bases;
}

int dump_process::get_all_hashes(unordered_set<unsigned __int64>* output_hashes, unordered_set<unsigned __int64>* output_hashes_eps, unordered_set<unsigned __int64>* output_hashes_ep_shorts, work_pool* pool)
{
	// Adds all the modules in the process to the output array
	if( _ph != NULL )
	{
		if( !_options->DumpChunks || build_export_list(pool) ) // Only build export list if getting hashes for code chunks
		{
			// First build a list of the modules
			module_list module_snapshot(_pid);
			auto* modules = &module_snapshot;
			
			set<unsigned __int64> executable_heaps;
			const auto bases = scan_regions(executable_heaps);
			std::mutex results;
			module_work(pool, bases, [&](unsigned __int64 base) {
				pe_header header(_ph, reinterpret_cast<void*>(base), modules, _options);
				if (!header.process_pe_header() || !header.process_sections() || !header.somewhat_parsed())
					return;
				const auto hash = header.get_hash();
				const auto short_hash = output_hashes_ep_shorts ? header.get_hash_ep_short() : 0;
				const auto full_hash = output_hashes_ep_shorts && output_hashes_eps ? header.get_hash_ep() : 0;
				const bool known = _db_clean->contains(hash);
				std::lock_guard<std::mutex> lock(results);
				exclude_module_heaps(executable_heaps, base, header.get_virtual_size());
				if (hash && !known) output_hashes->insert(hash);
				if (short_hash) output_hashes_ep_shorts->insert(short_hash);
				if (full_hash) output_hashes_eps->insert(full_hash);
			});

			
			if( _options->DumpChunks )
			{
				// One last loop to add the hashes from all stray executable heaps
				if( _options->Verbose )
					fprintf( stdout, "INFO: Looking at unattached executable heaps...\n" );
				
				int count_new_header_hashes = 0;
				std::vector<unsigned __int64> chunks;
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
						chunks.push_back(*it);
					}
				}
				module_work(pool, chunks, [&](unsigned __int64 base) {
					pe_header header(_ph, reinterpret_cast<void*>(base), modules, _options);
					if (!header.build_pe_header(0x1000, true, 1) || !header.process_sections())
						return;
					const auto summary = header.get_imports_information(&_export_list);
					if (summary.HASH_GENERIC && !_db_clean->contains(summary.HASH_GENERIC))
					{
						std::lock_guard<std::mutex> lock(results);
						output_hashes->insert(summary.HASH_GENERIC);
					}
				});
				if( _options->Verbose )
					fprintf( stdout, "INFO: Done looking at unattached executable heaps...\n" );
			}

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

bool dump_process::build_export_list(work_pool* pool)
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
			module_list modules(_pid);
			std::vector<unsigned __int64> bases;
			for (const auto& item : modules._modules) bases.push_back(item.first);
			std::vector<size_t> indices;
			for (size_t i = 0; i < bases.size(); ++i) indices.push_back(i);
			std::vector<std::unique_ptr<export_list>> exports(bases.size());
			module_work(pool, indices, [&](size_t i) {
				pe_header header(_ph, reinterpret_cast<void*>(bases[i]), &modules, _options);
				if (header.process_pe_header() && header.process_sections() && header.process_export_directory())
				{
					exports[i].reset(new export_list);
					exports[i]->take_exports(*header.get_exports());
				}
			});
			// Merge in snapshot order, not completion order, to preserve alias precedence.
			for (auto& entry : exports)
				if (entry) _export_list.take_exports(*entry);
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
			if (strcmpi(item->second->short_name, library) == 0)
			{
				pe_header* header = new pe_header(_ph, (void*)item->first, modules, _options);
				if (header->process_pe_header() && header->process_sections() && header->process_export_directory())
				{
					// Load its exports
					result->take_exports(*header->get_exports());
				}

				// Cleanup
				delete header;
			}
		}
	}


	return true;
}

bool dump_process::write_dump(pe_header* header, unsigned __int64 base, DWORD pid)
{
	const char* extension = header->is_exe() ? "exe" : (header->is_dll() ? "dll" : (header->is_sys() ? "sys" : "bin"));
	std::string filename, error;
	if (!dump_path::make_filename(_options->output_path, _process_name, pid, header->get_name(),
		base, header->is_64(), extension, filename, error))
	{
		fprintf(stderr, "ERROR: Cannot name dump for PID 0x%x at %llX: %s.\n", pid, base, error.c_str());
		return false;
	}
	printf(" dumping '%s' at %llX to file '%s'\n", extension, base, filename.c_str());
	if (!header->write_image(&filename[0]))
	{
		fprintf(stderr, "ERROR: Failed to write dump for PID 0x%x at %llX to '%s'.\n", pid, base, filename.c_str());
		return false;
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
						write_dump(header, base, pid);
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
			pe_header* header = new pe_header( _ph, (void*) base, modules, _options );
			
			if( _options->ForceGenHeader || !header->process_pe_header() )
			{
				if( _options->Verbose )
					printf( "Generating 64-bit PE header for module at %llX.\n", base );
				
				// Build the pe header as 32 and 64 bit since it could be either
				if (header->build_pe_header(0x1000, true))
					dump_header(header, base, _pid);
				delete header;

				if( _options->Verbose )
					printf( "Generating 32-bit PE header for module at %llX.\n", base );
				header = new pe_header( _ph, (void*) base, modules, _options );
				if (header->build_pe_header(0x1000, false))
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
		build_export_list(exports, "KernelBase.dll", modules);
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
		if (!_term_hook->unhock_terminate())
			return false;
		delete _term_hook;
		_term_hook = NULL;
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
			return _term_hook->resume_terminate();
		}

		return false; // was not waiting
	}
	return false; // not hooked
}

void dump_process::dump_all(work_pool* pool)
{
	// Walk through the pages while dumping all MZ files that do not match our good hash database.
	printf( "dumping process %s with pid 0x%x...\n", this->_process_name, this->_pid );
	if( _ph != NULL )
	{
		// First build the export list for this process
		if ((!_options->ImportRec && !_options->DumpChunks) || build_export_list(pool))
		{
			// First build a list of the modules
			module_list module_snapshot(_pid);
			auto* modules = &module_snapshot;

			set<unsigned __int64> executable_heaps;
			const auto bases = scan_regions(executable_heaps);
			std::mutex results;
			module_work(pool, bases, [&](unsigned __int64 base) {
				pe_header header(_ph, reinterpret_cast<void*>(base), modules, _options);
				if (header.process_pe_header() && header.process_sections() &&
					header.somewhat_parsed() && header.process_import_directory())
				{
					{
						std::lock_guard<std::mutex> lock(results);
						exclude_module_heaps(executable_heaps, base, header.get_virtual_size());
					}
					const auto hash = header.get_hash();
					if (hash != 0 && !_db_clean->contains(hash))
					{
						if (_options->ForceGenHeader)
						{
							printf("Dumping a module but ignoring existing PE Header for module at 0x%llX.\n", base);
							for (bool win64 : {true, false})
							{
								pe_header generated(_ph, reinterpret_cast<void*>(base), modules, _options);
								if (generated.build_pe_header(0x1000, win64))
									dump_header(&generated, base, _pid);
							}
						}
						else if (header.process_disk_image(&_export_list, _db_clean))
							write_dump(&header, base, _pid);
					}
				}
			});

			if( _options->DumpChunks )
			{
				// One last loop to add the hashes from all stray executable heaps
				if( _options->Verbose )
					fprintf( stdout, "INFO: Looking at unattached executable heaps...\n" );

				int count_new_header_hashes = 0;
				std::vector<unsigned __int64> chunks;
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
						
						chunks.push_back(*it);
					}
				}
				module_work(pool, chunks, [&](unsigned __int64 base) {
					pe_header header(_ph, reinterpret_cast<void*>(base), modules, _options);
					if (!header.build_pe_header(0x1000, true, 1) || !header.process_sections())
						return;
					const auto summary = header.get_imports_information(&_export_list);
					if (summary.HASH_GENERIC && !_db_clean->contains(summary.HASH_GENERIC) &&
						header.somewhat_parsed() && summary.COUNT_UNIQUE_IMPORT_ADDRESSES >= 2)
					{
						printf("Dumping unattached executable code chunk from 0x%llX.\n", base);
						for (bool win64 : {true, false})
						{
							pe_header generated(_ph, reinterpret_cast<void*>(base), modules, _options);
							if (generated.build_pe_header(0x1000, win64))
							{
								generated.set_name("codechunk");
								dump_header(&generated, base, _pid);
							}
						}
					}
				});
				if( _options->Verbose )
					fprintf( stdout, "INFO: Done looking at unattached executable heaps...\n" );
			}

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
