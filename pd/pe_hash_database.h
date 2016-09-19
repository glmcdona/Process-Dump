#pragma once


#include <unordered_set>
#include <iostream>
#include <stdio.h>
#include <io.h>
#include <sys/types.h>
#include "dirent.h"
#include "Shlwapi.h"
#include "pe_header.h"
#include "simple.h"
#include <windows.h>          // std::mutex






using namespace std;
using namespace std::tr1;

class pe_hash_database
{
	char* _clean_database_path;
	unordered_set<unsigned __int64> _clean_hashes;
	bool _is_mz(FILE* stream);
	CRITICAL_SECTION _lock;

public:
	pe_hash_database(char* clean_database_name);
	int count();
	bool clear_database();
	bool add_hashes(unordered_set<unsigned __int64> hashes);
	bool add_folder( char* dir_name, WCHAR* filter, bool recursively );
	bool remove_folder( char* dir_name, WCHAR* filter, bool recursively );
	bool contains(unsigned __int64 hash);
	bool add_file(char* file);
	bool remove_file(char* file);
	bool save();
	~pe_hash_database(void);
};