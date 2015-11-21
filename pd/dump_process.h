#pragma once

// content
#include "pe_header.h"
#include "pe_hash_database.h"
#include "windows.h"
#include <tlhelp32.h>
#include "simple.h"
#include "module_list.h"
#include "DynArray.h"
#include "export_list.h"

#define PAGE_SIZE 0x1000

using namespace std;
using namespace std::tr1;

class dump_process
{
	pe_hash_database* _db_clean;
	bool _opened;
	HANDLE _ph;
	DWORD _pid;
	char* _process_name;
	HANDLE _hSnapshot;
	export_list _export_list;
	PD_OPTIONS _options;
	
	

public:
	dump_process(DWORD pid, pe_hash_database* db, PD_OPTIONS options);
	void dump_all();
	void dump_region(__int64 base);
	void dump_header(pe_header* header, __int64 base);
	bool build_export_list();
	int get_all_hashes(DynArray<unsigned __int64>* output_hashes);
	~dump_process(void);
};