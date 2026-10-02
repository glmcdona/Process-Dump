#pragma once

#include <stdio.h>
#include "windows.h"
#include "simple.h"
#include <tlhelp32.h>
#include <unordered_map>
#include "Psapi.h"
#include <limits.h>
#include <string>
#include <string.h>
#include <wchar.h>
#include <vector>

using namespace std::tr1;


extern bool global_flag_verbose;

namespace module_snapshot
{
	using enumerator = BOOL (WINAPI *)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD);

	inline bool read(HANDLE process, std::vector<HMODULE>& modules, enumerator enumerate = EnumProcessModulesEx)
	{
		modules.resize(2048);
		for (int attempt = 0; attempt < 8; ++attempt)
		{
			DWORD needed = 0;
			if (!enumerate(process, modules.data(), static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &needed, LIST_MODULES_ALL))
			{
				modules.clear();
				return false;
			}
			if (needed % sizeof(HMODULE) != 0 || needed / sizeof(HMODULE) > 1024 * 1024)
				break;
			if (needed / sizeof(HMODULE) <= modules.size())
			{
				modules.resize(needed / sizeof(HMODULE));
				return true;
			}
			modules.resize(needed / sizeof(HMODULE));
		}
		modules.clear();
		SetLastError(ERROR_BAD_LENGTH);
		return false;
	}
}

namespace module_names
{
	inline bool to_ansi(const wchar_t* source, SIZE_T source_count, std::string& result, const char* description)
	{
		result.clear();
		if (source == NULL || source_count == 0 || source_count > INT_MAX ||
			wmemchr(source, L'\0', source_count) == NULL)
		{
			fprintf(stderr, "WARNING: Unterminated %s.\n", description);
			return false;
		}
		const int required = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
		if (required == 0)
		{
			fprintf(stderr, "WARNING: Unable to convert %s (error %lu).\n", description, GetLastError());
			return false;
		}
		result.resize(static_cast<SIZE_T>(required));
		if (WideCharToMultiByte(CP_ACP, 0, source, -1, &result[0], required, NULL, NULL) != required)
		{
			result.clear();
			fprintf(stderr, "WARNING: Unable to convert %s (error %lu).\n", description, GetLastError());
			return false;
		}
		result.resize(static_cast<SIZE_T>(required) - 1);
		return true;
	}

	inline void copy_wide(const wchar_t* source, SIZE_T source_count, char* target, SIZE_T target_size, const char* description)
	{
		if (target == NULL || target_size == 0)
			return;
		target[0] = 0;
		std::string converted;
		if (!to_ansi(source, source_count, converted, description))
			return;
		if (converted.size() >= target_size)
		{
			fprintf(stderr, "WARNING: Converted %s exceeds the supported byte length.\n", description);
			return;
		}
		memcpy(target, converted.c_str(), converted.size() + 1);
	}

	inline void check_psapi_name(DWORD copied, char* target, DWORD capacity, const char* description)
	{
		if (target == NULL || capacity == 0)
			return;
		target[capacity - 1] = 0;
		if (copied == 0 || copied >= capacity)
		{
			target[0] = 0;
			fprintf(stderr, "WARNING: Unable to read a complete %s.\n", description);
		}
	}
}

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

		full_name = new char[260]();
		short_name = new char[256]();

		// Read in the short and long name
		module_names::check_psapi_name(GetModuleFileNameExA(ph, mh, full_name, 260), full_name, 260, "module path");
		module_names::check_psapi_name(GetModuleBaseNameA(ph, mh, short_name, 256), short_name, 256, "module name");
	}

	module( MODULEENTRY32 tmpModule )
	{
		this->start = (unsigned __int64) tmpModule.modBaseAddr;
		this->size = (unsigned __int64) tmpModule.modBaseSize;

		full_name = new char[260]();
		short_name = new char[256]();

		module_names::copy_wide(tmpModule.szExePath, _countof(tmpModule.szExePath), full_name, 260, "module path");
		module_names::copy_wide(tmpModule.szModule, _countof(tmpModule.szModule), short_name, 256, "module name");
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
