#pragma once


#include <unordered_set>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <io.h>
#include <sys/types.h>
#include "dirent.h"
#include "Shlwapi.h"
#include "pe_header.h"
#include "simple.h"
#include <windows.h>          // std::mutex
#include "work_pool.h"






using namespace std;
using namespace std::tr1;

class pe_hash_database
{
public:
	struct entrypoint_hashes
	{
		unordered_set<unsigned __int64> full;
		unordered_set<unsigned __int64> short_hashes;
		unsigned __int64 minimum = _UI64_MAX;
		unsigned __int64 maximum = 0;
	};
private:
	char* _clean_database_path; // Known clean modules
	char* _ep_database_path; // EntryPoint hashes
	char* _epshort_database_path; // EntryPoint short hashes

	unordered_set<unsigned __int64> _clean_hashes;
	unordered_set<unsigned __int64> _ep_hashes;
	unordered_set<unsigned __int64> _epshort_hashes;
	std::shared_ptr<const entrypoint_hashes> _entrypoint_snapshot;

	bool _is_mz(FILE* stream);
	bool _add_folder(char* dir_name, WCHAR* filter, bool recursively, work_pool& pool, size_t& queued);
	CRITICAL_SECTION _lock;

public:
	std::shared_ptr<const entrypoint_hashes> snapshot_entrypoints();
	pe_hash_database(char* clean_database_name, char* ep_database_name, char* epshort_database_name);

	int count();
	int count_eps();
	int count_epshorts();

	bool clear_database();

	bool add_hashes(const unordered_set<unsigned __int64>& hashes);
	bool add_hashes_eps(const unordered_set<unsigned __int64>& hashes, const unordered_set<unsigned __int64>& hashes_short);

	bool add_folder(char* dir_name, WCHAR* filter, bool recursively, int threads = 1);
	bool remove_folder( char* dir_name, WCHAR* filter, bool recursively );
	bool contains(unsigned __int64 hash);
	bool contains_epshort(unsigned __int64 hash);
	bool contains_ep(unsigned __int64 hash);
	bool add_file(char* file);
	bool remove_file(char* file);
	bool save();
	~pe_hash_database(void);
};