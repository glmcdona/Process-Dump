#pragma once

#include <windows.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include "windows.h"
#include <tlhelp32.h>
#include <Psapi.h>
#include <regex>
#include "DynArray.h"

using namespace std;
using namespace std::tr1;

struct PD_OPTIONS
{
	bool ImportRec;
	bool ForceGenHeader;
	bool Verbose;
};

class process_description
{
public:
	char* process_name;
	DWORD pid;

	process_description(char* name, DWORD pid)
	{
		process_name = new char[strlen(name)+1];
		strcpy( process_name, name );
		this->pid = pid;
	}
	
	~process_description()
	{
		delete[] process_name;
	}
};

DWORD process_find(string match_regex, DynArray<process_description*>* result);
string ExePath();
void PrintLastError(LPTSTR lpszFunction); 