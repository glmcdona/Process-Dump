#include "StdAfx.h"
#include "module_list.h"


module_list::module_list()
{
	// Empty list
}

module_list::module_list( DWORD pid )
{
	#if defined(_WIN64)
	// List modules on a 64 bit machine. A 64 bit machine is assumed to be Windows Vista+
	HMODULE hMods[2048];
	DWORD cbNeeded;
    unsigned int i;
	HANDLE ph = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                            FALSE, pid );
	if( ph != NULL )
	{
		if( EnumProcessModulesEx(ph, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
		{
			for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
			{
				// Query the module basic information
				MODULEINFO info;
				if( GetModuleInformation( ph, hMods[i], &info, sizeof(MODULEINFO) ) )
				{
					// Check if this base address is already occupied
					unordered_map<unsigned __int64, module*>::const_iterator item = _modules.find( (unsigned __int64) info.lpBaseOfDll );

					if( item == _modules.end() )
					{
						// Add this module
						_modules[(unsigned __int64) info.lpBaseOfDll ] = new module( ph, hMods[i], info );
					}
				}
				else
				{
					PrintLastError(L"module_list GetModuleInformation");
				}
			}
		}

		CloseHandle( ph );				
	}
	else
	{
		if( GetLastError() == 299 )
			fprintf(stderr, "ERROR: Unable to open process PID 0x%x since it is a 64 bit process and this tool is running as a 32 bit process.\r\n", pid);
		else
			PrintLastError(L"module_list OpenProcess");
	}
	#elif defined(_WIN32)

	HANDLE hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if ( hSnapshot == INVALID_HANDLE_VALUE )
	{
		if( global_flag_verbose )
			printf ("WARNING: Could not gather process information for process pid 0x%X, error code (%d).\r\n", pid, GetLastError());
		return;
	}

	MODULEENTRY32 tmpModule;
	tmpModule.dwSize = sizeof(MODULEENTRY32);
	if( Module32First(hSnapshot, &tmpModule) )
	{
		// Add this first module to our array
		tmpModule.dwSize = sizeof(MODULEENTRY32);

		// Check if this base address is already occupied
		unordered_map<unsigned __int64, module*>::const_iterator item = _modules.find( (unsigned __int64) tmpModule.modBaseAddr );
		if( item == _modules.end() )
		{
			// Add this module
			_modules[(unsigned __int64) tmpModule.modBaseAddr ] = new module( tmpModule );
		}

		while(Module32Next(hSnapshot,&tmpModule))
		{
			// Add this module to our array
			unordered_map<unsigned __int64, module*>::const_iterator item = _modules.find( (unsigned __int64) tmpModule.modBaseAddr );
			if( item == _modules.end() )
			{
				// Add this module
				_modules[(unsigned __int64) tmpModule.modBaseAddr ] = new module( tmpModule );
			}

			tmpModule.dwSize = sizeof(MODULEENTRY32);
		}
	}

	CloseHandle(hSnapshot);

	#endif
}

module_list::~module_list(void)
{
	// Clean up the module list
	for ( unordered_map<unsigned __int64, module*>::const_iterator item = _modules.begin(); item != _modules.end(); ++item )
		delete item->second;
}
