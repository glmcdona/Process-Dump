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
#include "hash.h"
#include <set>
#include "terminate_monitor_hook.h"

#define PAGE_SIZE 0x1000
#define CODECHUNK_HEADER_HASH_SIZE 0x200 // First X bytes are CRC32'd of each loose code chunk. Only unique CRC32s are processed deeply.
#define CODECHUNK_NEW_HASH_LIMIT 500 // At most X code chunks processed deeply per process

using namespace std;
using namespace std::tr1;


struct MBI_BASIC_INFO
{
	__int64 base;
	__int64 end;
	DWORD protect;
	bool valid;
	bool executable;
};

class dump_process
{
	pe_hash_database* _db_clean;
	bool _opened;
	HANDLE _ph;
	DWORD _pid;
	char* _process_name;
	export_list _export_list;
	bool _export_list_built;
	PD_OPTIONS* _options;
	terminate_monitor_hook* _term_hook;

	unsigned __int64 _address_main_module = NULL;

	bool _loaded_is64;
	bool _is64;
	bool _quieter; // Suppress some of the error and warning messages

	MBI_BASIC_INFO get_mbi_info(unsigned __int64 address);

public:
	dump_process(DWORD pid, pe_hash_database* db, PD_OPTIONS* options, bool quieter);
	void dump_all();
	void dump_region(__int64 base);
	void dump_header(pe_header* header, __int64 base, DWORD pid);
	DWORD get_pid() { return _pid; };
	bool build_export_list();
	bool build_export_list(export_list* result, char* library, module_list* modules);
	int get_all_hashes(unordered_set<unsigned __int64>* output_hashes);
	unsigned __int64 hash_codechunk_header(__int64 base);
	bool is64();
	bool get_process_name(char* process_name, SIZE_T byte_length);

	// Functions for dumping processes as they close
	bool monitor_close_start(); // start terminate hooks
	bool monitor_close_is_waiting(); // check if the process has closed and is waiting dumping
	bool monitor_close_dump_and_resume(); // dumps the process if it is waiting dumping
	bool monitor_close_stop(); // stop terminate hooks

	~dump_process(void);
};