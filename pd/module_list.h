#pragma once

#include <stdio.h>
#include "windows.h"
#include "simple.h"
#include <tlhelp32.h>
#include <unordered_map>
#include "Psapi.h"

using namespace std::tr1;


extern bool global_flag_verbose;

class module
{
	public:
	unsigned __int64 start;
	unsigned __int64 size;
	char* full_name;
	char* short_name;

	module( HANDLE ph, HMODULE mh, MODULEINFO info )
	{
		this->start = (unsigned __int64) info.lpBaseOfDll;
		this->size = (unsigned __int64) info.SizeOfImage;

		full_name = new char[260];
		short_name = new char[256];

		// Read in the short and long name
		GetModuleFileNameExA( ph, mh, full_name, 260 );
		GetModuleBaseNameA( ph, mh, short_name, 256 );
	}

	module( MODULEENTRY32 tmpModule )
	{
		this->start = (unsigned __int64) tmpModule.modBaseAddr;
		this->size = (unsigned __int64) tmpModule.modBaseSize;

		full_name = new char[260];
		short_name = new char[256];

		_snprintf( full_name, 259, "%S", tmpModule.szExePath );
		_snprintf( short_name, 255, "%S", tmpModule.szModule );
	}

	~module()
	{
		if( full_name != NULL )
			delete[] full_name;
		if( short_name != NULL )
			delete[] short_name;
	}
};

class module_list
{
	
	HANDLE _ph;

public:
	unordered_map <unsigned __int64, module*> _modules;
	module_list();
	module_list( DWORD pid );
	~module_list(void);
};
