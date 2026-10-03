// pd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "pe_header.h"
#include <tlhelp32.h>
#include <cstdio>
#include "pe_hash_database.h"
#include "dump_process.h"
#include "simple.h"
#include "work_queue.h"
#include <thread>
#include "pd.h"
#include "close_watcher.h"
#include "system_work.h"
#include "version.h"

#define NMD_ASSEMBLY_IMPLEMENTATION
#include "nmd_assembly.h"


BOOL is_win64()
{
	#if defined(_WIN64)
		return TRUE;  // 64-bit programs run only on Win64
	#elif defined(_WIN32)
		// 32-bit programs run on both 32-bit and 64-bit Windows
		// so must sniff
		BOOL f64 = FALSE;
		return IsWow64Process(GetCurrentProcess(), &f64) && f64;
	#else
		return FALSE; // Win64 does not support Win16
	#endif
}

bool is_elevated(HANDLE h_Process)
{
	HANDLE h_Token;
	TOKEN_ELEVATION t_TokenElevation;
    TOKEN_ELEVATION_TYPE e_ElevationType;
	DWORD dw_TokenLength;
	
	if( OpenProcessToken(h_Process, TOKEN_READ | TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES , &h_Token) )
	{
		if(GetTokenInformation(h_Token,TokenElevation,&t_TokenElevation,sizeof(t_TokenElevation),&dw_TokenLength))
		{
			if(t_TokenElevation.TokenIsElevated != 0)
			{
				if(GetTokenInformation(h_Token,TokenElevationType,&e_ElevationType,sizeof(e_ElevationType),&dw_TokenLength))
				{
					if(e_ElevationType == TokenElevationTypeFull || e_ElevationType == TokenElevationTypeDefault)
					{
						return true;
					}
				}
			}
		}
	}

    return false;
}


bool get_privileges(HANDLE h_Process)
{
	HANDLE h_Token;
	DWORD dw_TokenLength;
	if( OpenProcessToken(h_Process, TOKEN_READ | TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES , &h_Token) )
	{
		// Read the old token privileges
		TOKEN_PRIVILEGES* privilages = new TOKEN_PRIVILEGES[100];
		if( GetTokenInformation(h_Token, TokenPrivileges, privilages,sizeof(TOKEN_PRIVILEGES)*100,&dw_TokenLength) )
		{
			// Enable all privileges
			for( int i = 0; i < privilages->PrivilegeCount; i++ )
			{
				privilages->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
			}
			
			// Adjust the privilges
			if(AdjustTokenPrivileges( h_Token, false, privilages, sizeof(TOKEN_PRIVILEGES)*100, NULL, NULL  ))
			{
				delete[] privilages;
				return true;
			}
		}
		delete[] privilages;
	}
	return false;
}

bool ConsoleRequestingClose = false;
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	char mesg[128];

	switch (CEvent)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		// Cancel the event and set closing flag
		printf("Close request received.\n");
		ConsoleRequestingClose = true;
		break;
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		// We need to cleanup before terminating since we can't cancel this event
		printf("Terminate request received.\n");
		ConsoleRequestingClose = true;
		Sleep(30000); // hackjob to make sure it cleans up
		break;
	}
	return TRUE;
}

void add_process_hashes( DWORD pid, pe_hash_database* db, PD_OPTIONS* options )
{
	// Build a list of the hashes from this process
	unordered_set<unsigned __int64> new_hashes;
	unordered_set<unsigned __int64> new_hashes_eps;
	unordered_set<unsigned __int64> new_hashes_ep_shorts;

	// Process this process
	dump_process* dumper = new dump_process( pid, db, options, true );
	dumper->get_all_hashes( &new_hashes, &new_hashes_eps, &new_hashes_ep_shorts);
	delete dumper;
	
	// Add all these hashes to the database
	db->add_hashes( new_hashes );
	db->add_hashes_eps( new_hashes_eps, new_hashes_ep_shorts);
}

void add_system_hashes( pe_hash_database* db, PD_OPTIONS* options )
{
	work_pool pool(options->NumberOfThreads);
	system_work(pool, false, system_processes, [=](DWORD pid, work_pool& workers) {
		unordered_set<unsigned __int64> hashes, full, prefixes;
		dump_process dumper(pid, db, options, true);
		dumper.get_all_hashes(&hashes, &full, &prefixes, &workers);
		db->add_hashes(hashes);
		db->add_hashes_eps(full, prefixes);
	});
}


void dump_system(pe_hash_database* db, PD_OPTIONS* options)
{
	work_pool pool(options->NumberOfThreads);
	system_work(pool, true, system_processes, [=](DWORD pid, work_pool& workers) {
		dump_process dumper(pid, db, options, true);
		dumper.dump_all(&workers);
		unordered_set<unsigned __int64> hashes;
		dumper.get_all_hashes(&hashes, NULL, NULL, &workers);
		db->add_hashes(hashes);
	});
}






bool global_flag_verbose = false;

int _tmain(int argc, _TCHAR* argv[]) try
{

	// Process the flags	
	WCHAR* filter = NULL;
	char* processNameFilter = NULL;
	char* clean_database;
	char* ep_database;
	char* epshort_database;
	string path = ExePath();

	clean_database = new char[ path.length() + strlen("clean.hashes") + 2 ];
	sprintf( clean_database, "%s\\%s", path.c_str() , "clean.hashes" );

	ep_database = new char[path.length() + strlen("entrypoints.hashes") + 2];
	sprintf(ep_database, "%s\\%s", path.c_str(), "entrypoints.hashes");

	epshort_database = new char[path.length() + strlen("shortentrypoints.hashes") + 2];
	sprintf(epshort_database, "%s\\%s", path.c_str(), "shortentrypoints.hashes");
	

	bool flagHelp = false;
	bool flagHeader = true;
	bool flagPidDump = false;
	bool flagProcessNameDump = false;
	bool flagSystemDump = false;
	bool flagAddressDump = false;
	bool flagDumpCloses = false;

	char* add_directory = NULL;
	bool flagDB_gen = false;
	bool flagDB_genQuick = false;
	bool flagDB_add = false;
	bool flagDB_clean = false;
	bool flagDB_ignore = false;
	bool flagDB_remove = false;
	bool flagRecursion = true;
	
	
	PD_OPTIONS options;
	options.ImportRec = true;
	options.ForceGenHeader = false;
	options.Verbose = false;
	options.EntryPointOverride = -1;
	options.ReconstructHeaderAsDll = false;
	options.DumpChunks = true;
	options.EntryPointHash = true;
	options.NumberOfThreads = 16; // Default 16 threads
	options.ForceReconstructEntryPoint = false;
	
	DWORD pid = -1;
	__int64 address = 0;

	if( argc <= 1 )
		flagHelp = true;

	for( int i = 1; i < argc; i++ )
	{
		if( lstrcmp(argv[i],L"--help") == 0 || lstrcmp(argv[i],L"-help") == 0 || lstrcmp(argv[i],L"-h") == 0 || lstrcmp(argv[i],L"--h") == 0)
			flagHelp = true;
		else if( lstrcmp(argv[i],L"-nh") == 0 )
			flagHeader = false;
		else if( lstrcmp(argv[i],L"-nr") == 0 )
			flagRecursion = false;
		else if( lstrcmp(argv[i],L"-ni") == 0 )
			options.ImportRec = false;
		else if (lstrcmp(argv[i], L"-reexec") == 0)
			options.Reexecution = true;
		else if( lstrcmp(argv[i],L"-nc") == 0 )
			options.DumpChunks = false;
		else if (lstrcmp(argv[i], L"-nep") == 0)
			options.EntryPointHash = false;
		else if (lstrcmp(argv[i], L"-nt") == 0)
			options.NumberOfThreads = 1;
		else if (lstrcmp(argv[i], L"-eprec") == 0)
			options.ForceReconstructEntryPoint = true;
		else if (lstrcmp(argv[i], L"-closemon") == 0)
			flagDumpCloses = true;
		else if( lstrcmp(argv[i],L"-v") == 0 )
		{
			options.Verbose = true;
			global_flag_verbose = true;
		}
		else if( lstrcmp(argv[i],L"-g") == 0 )
			options.ForceGenHeader = true;
		else if( lstrcmp(argv[i],L"-pid") == 0 )
		{
			if( i + 1 < argc )
			{
				// Attempt to parse this second part
				filter = argv[i+1];

				// Check the prefix
				bool isHex = false;
				wchar_t* prefix = new wchar_t[3];
				memcpy(prefix, filter, 4);
				prefix[2] = 0;

				if( wcscmp(prefix, L"0x") == 0 )
				{
					filter = &filter[2];
					isHex = true;
				}
				delete[] prefix;
				
				// Extract the pid from the string
				if( (isHex && swscanf(filter, L"%x", &pid) > 0) ||
					(!isHex && swscanf(filter, L"%i", &pid) > 0))
				{
					// Successfully parsed the PID
					flagPidDump = true;
				}
				else
				{
					fprintf(stderr,"Failed to parse -pid argument. It must be followed by a number:\n\teg. 'pd -pid 0x10A'\n");
					exit(0);
				}

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -pid argument. It must be followed by a number:\n\teg. 'pd -pid 0x10A'\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-a") == 0 )
		{
			if( i + 1 < argc )
			{
				// Attempt to parse this second part
				filter = argv[i+1];

				// Check the prefix
				bool isHex = false;
				wchar_t* prefix = new wchar_t[3];
				memcpy(prefix, filter, 4);
				prefix[2] = 0;

				if( wcscmp(prefix, L"0x") == 0 )
				{
					filter = &filter[2];
					isHex = true;
				}
				delete[] prefix;
				
				// Extract the pid from the string
				if( (isHex && swscanf(filter, L"%llx", &address) > 0) ||
					(!isHex && swscanf(filter, L"%llu", &address) > 0))
				{
					// Successfully parsed the PID
					flagAddressDump = true;
				}
				else
				{
					fprintf(stderr,"Failed to parse -a address argument. It must be followed by a number:\n\teg. 'pd -a 0x401000 -pid 0x10A'\n");
					exit(0);
				}

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -pid argument. It must be followed by a number:\n\teg. 'pd -pid 0x10A'\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-p") == 0 )
		{
			if( i + 1 < argc )
			{
				// Extract the process name filter regex
				processNameFilter = new char[wcslen(argv[i+1]) + 1];
				sprintf( processNameFilter, "%S", argv[i+1] );
				
				flagProcessNameDump = true;
				
				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -p argument. It must be followed by a regex match statement:\n\teg. 'pd -p chrome.exe'\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-t") == 0 )
		{
			if( i + 1 < argc )
			{
				// Extract the number of threads to use
				filter = argv[i+1];

				// Check the prefix
				bool isHex = false;
				wchar_t* prefix = new wchar_t[3];
				memcpy(prefix, filter, 4);
				prefix[2] = 0;

				if( wcscmp(prefix, L"0x") == 0 )
				{
					filter = &filter[2];
					isHex = true;
				}
				delete[] prefix;
				
				// Extract the number from the string
				if( (isHex && swscanf(filter, L"%x", &options.NumberOfThreads) > 0) ||
					(!isHex && swscanf(filter, L"%i", &options.NumberOfThreads) > 0))
				{
					// Successfully parsed the value
					if( options.NumberOfThreads < 1 )
					{
						fprintf(stderr,"Failed to parse -t argument. It must be followed by a number 1 or larger:\n\teg. 'pd -system -t 10'\n");
						exit(0);
					}
					printf("Set number of threads to %i.\n", options.NumberOfThreads);
				}
				else
				{
					fprintf(stderr,"Failed to parse -t argument. It must be followed by a number:\n\teg. 'pd -system -t 10'\n");
					exit(0);
				}
				
				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -t argument. It must be followed by a number:\n\teg. 'pd -system -t 10'\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-c") == 0 || lstrcmp(argv[i], L"-cdb") == 0)
		{
			if( i + 1 < argc )
			{
				// Extract the path to use as the clean file database
				clean_database = new char[wcslen(argv[i+1]) + 1];
				sprintf( clean_database, "%S", argv[i+1] );
				printf("Set clean database filepath to %s.\n", clean_database);
				
				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -c argument. It must be followed by a path to the clean file hash database to use.\n");
				exit(0);
			}
		}
		else if (lstrcmp(argv[i], L"-edb") == 0)
		{
			if (i + 1 < argc)
			{
				// Extract the path to use as the clean file database
				ep_database = new char[wcslen(argv[i + 1]) + 1];
				sprintf(ep_database, "%S", argv[i + 1]);
				printf("Set entrypoint database filepath to %s.\n", ep_database);

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr, "Failed to parse -edb argument. It must be followed by a path to the entrypoint file hash database to use.\n");
				exit(0);
			}
		}
		else if (lstrcmp(argv[i], L"-esdb") == 0)
		{
			if (i + 1 < argc)
			{
				// Extract the path to use as the clean file database
				epshort_database = new char[wcslen(argv[i + 1]) + 1];
				sprintf(epshort_database, "%S", argv[i + 1]);
				printf("Set entrypoint short database filepath to %s.\n", epshort_database);

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr, "Failed to parse -esdb argument. It must be followed by a path to the entrypoint short file hash database to use.\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-o") == 0 )
		{
			if( i + 1 < argc )
			{
				// Extract the default output path to use
				char* output_path = new char[wcslen(argv[i+1]) + 1];
				sprintf( output_path, "%S", argv[i+1] );
				options.set_output_path( output_path );
				printf("Set output path to %s.\n", output_path);
				delete [] output_path;
				
				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -c argument. It must be followed by a path to the clean file hash database to use.\n");
				exit(0);
			}
		}
		else if( lstrcmp(argv[i],L"-system") == 0 )
			flagSystemDump = true;
		else if( lstrcmp(argv[i],L"-db") == 0 )
		{
			// Process the db commands
			if(  i + 1 < argc )
			{
				// Process this db command
				// -db gen
				// -db genquick
				// -db add [directory]
				// -db clean
				// -db ignore
				if( lstrcmp(argv[i+1],L"gen") == 0 )
				{
					flagDB_gen = true;
				}else if( lstrcmp(argv[i+1],L"genquick") == 0 )
				{
					flagDB_genQuick = true;
				}else if( lstrcmp(argv[i+1],L"clean") == 0 )
				{
					flagDB_clean = true;
				}else if( lstrcmp(argv[i+1],L"ignore") == 0 )
				{
					flagDB_ignore = true;
				}else if( lstrcmp(argv[i+1],L"add") == 0 )
				{
					// Has yet another argument to specify the directory to add
					if(  i + 2 < argc )
					{
						// Extract the directory name to add
						add_directory = new char[wcslen(argv[i+2]) + 2];
						sprintf( add_directory, "%S", argv[i+2] );

						// Add a trailing slash if it doesn't exist
						if (add_directory[strlen(add_directory) - 1] != '\\' && add_directory[strlen(add_directory) - 1] != '//')
						{
							add_directory[strlen(add_directory) + 1] = 0;
							add_directory[strlen(add_directory)] = '\\';
						}

						DIR* pdir = opendir( add_directory );
						if( pdir != NULL )
						{
							flagDB_add = true;
							closedir( pdir );
						}
						else
						{
							fprintf(stderr,"Failed to process '-db add' argument. The directory '%s' does not exist or is not accessible.\n", add_directory);
							exit(0);
						}
					}
					else
					{
						fprintf(stderr,"Failed to parse '-db add' argument. It must be followed by a directory to add:\n\teg. 'pd -db add C:\\Windows\\'\n");
						exit(0);
					}
					i+=1;
				}else if( lstrcmp(argv[i+1],L"remove") == 0 || lstrcmp(argv[i+1],L"rem") == 0 )
				{
					// Has yet another argument to specify the directory to remove
					if(  i + 2 < argc )
					{
						// Extract the directory name to remove
						add_directory = new char[wcslen(argv[i+2]) + 2];
						sprintf( add_directory, "%S", argv[i+2] );

						// Add a trailing slash if it doesn't exist
						if (add_directory[strlen(add_directory) - 1] != '\\' && add_directory[strlen(add_directory) - 1] != '//')
						{
							add_directory[strlen(add_directory)+1] = 0;
							add_directory[strlen(add_directory)] = '\\';
						}

						DIR* pdir = opendir( add_directory );
						if( pdir != NULL )
						{
							flagDB_remove = true;
							closedir( pdir );
						}
						else
						{
							fprintf(stderr,"Failed to process '-db remove' argument. The directory '%s' does not exist or is not accessible.\n", add_directory);
							exit(0);
						}
					}
					else
					{
						fprintf(stderr,"Failed to parse '-db remove' argument. It must be followed by a directory to remove:\n\teg. 'pd -db add C:\\Windows\\'\n");
						exit(0);
					}
					i+=1;
				}


				i+=1;
			}else{
				fprintf(stderr,"Failed to parse -db argument. It must be followed by a command:\n\teg. 'pd -db genquick'\n");
				exit(0);
			}
		}else{
			// This is an unassigned argument
			fprintf(stderr,"Failed to parse argument number %i, '%S'. Try 'pd --help' for usage instructions.\n", i, argv[i]);
			exit(0);
		}
	}

	if( flagHeader )
	{
		printf("Process Dump v%s\n", PD_VERSION_STRING);
		printf("  Copyright © 2017, Geoff McDonald\n");
		printf("  http://www.split-code.com/\n");
		printf("  https://github.com/glmcdona/Process-Dump\n\n");
	}

	if( flagHelp )
	{
		fputs(R"help(Usage: pd.exe <command> [options]

Reconstruct PE32/PE64 modules, hidden modules and loose executable chunks from
process memory for analysis. Dumps are not guaranteed runnable or signed.
Build a baseline database on a known-clean system before collecting unknown code.
Use pd64.exe on 64-bit Windows, pd32.exe on 32-bit Windows; local builds use pd.exe.
Administrator rights improve access but do not bypass protected processes.

Commands (choose one; run database maintenance separately from dumping):
  -system              Dump unknown modules/chunks from all accessible processes.
                       Workers share module jobs, including the last process;
                       one final scan checks for newly discovered PIDs.
  -pid <pid>           Dump one process. PID may be decimal or 0x-prefixed hex.
  -p <regex>           Match the entire process name, case-sensitive (ECMAScript).
                       Quote the expression; multiple matches prompt for approval.
  -closemon            Hook termination of accessible processes and dump before
                       they exit. Intrusive; use only in a controlled environment.
                       Press Ctrl+C to stop and clean up hooks.
  -db gen              Add live hashes and PE files recursively from %WINDIR%,
                       %USERPROFILE%, C:\Program Files, C:\Program Files (x86).
  -db genquick         Add live module/chunk and entrypoint hashes only.
  -db add <dir>        Add PE file hashes recursively from a directory.
  -db remove <dir>     Remove matching clean-module hashes from a directory;
  -db rem <dir>        alias of remove. Entrypoint signatures are retained.
  -db clean            Clear and save ALL THREE databases, including entrypoints.

Dump/reconstruction options:
  -a <address>         Dump at a decimal or 0x-prefixed base address; requires -pid.
                       Reuse a valid PE header; absent headers or -g generate
                       both PE32 and PE64 analysis images. Clean filtering applies.
  -o <path>            Output directory (default: current working directory).
                       Use an existing ordinary directory you control. Existing
                       files are never overwritten; reparse paths are rejected.
  -g                   Force generated PE32 and PE64 headers instead of originals.
  -ni                  Disable aggressive import reconstruction (default: on).
  -nc                  Disable loose-code dumping/hashing (default: on).
  -nep                 Disable entrypoint recovery and live entrypoint hashing.
                       File database add/gen still collect entrypoint signatures.
  -eprec               Force recovery even for an existing executable entrypoint.
                       Default: preserve valid entrypoints. Full opcode matches
                       are required; x64 runtime-function metadata is a fallback.
                       No weak-prefix-only guesses; failure retains the old RVA.
                       -nep takes precedence. Recovery remains heuristic.
  -reexec              Experimental fresh-launch preparation (default: off).
                       Reset the GS cookie and writable/non-executable zero-fill
                       tails; restrict new imports to file-backed writable data
                       outside delay-IAT sections. Assumes original section layout
                       and discards captured state. Does not run the dump, restore
                       a checkpoint, or disable ASLR/DEP/CFG.

Database options:
  -db ignore           Ignore ALL THREE loaded databases for this dump invocation,
                       including entrypoint signatures; files are not cleared.
                       In-run cross-process deduplication can still skip repeats.
  -nr                  Disable recursion for -db add/remove/rem (not -db gen).
  -cdb <filepath>      Clean-module database (alias: -c <filepath>).
  -edb <filepath>      Full entrypoint-opcode database.
  -esdb <filepath>     Short entrypoint-prefix database.
                       Defaults beside pd.exe: clean.hashes, entrypoints.hashes,
                       shortentrypoints.hashes. Maintenance updates these files.
                       Hash matches are exclusions, not a trust verdict.

Workers and display:
  -t <count>           Worker count, at least 1 (default: 16). Applies to -system,
                       live/file database generation and close-monitor workers.
                       Direct -pid/-p dumps remain synchronous.
  -nt                  Use one worker (same as -t 1). Last -t/-nt wins.
                       Close monitoring still has a separate monitoring thread.
  -v                   Verbose diagnostics (default: off).
  -nh                  Suppress the version/copyright banner, not other output.
  --help               Print help and exit; aliases: -help, -h, --h.
                       No arguments also prints help.

Examples (create the output directory first):
  pd.exe -db genquick -t 4
  pd.exe -system -t 4 -o C:\dumps
  pd.exe -pid 419
  pd.exe -pid 0x1a3 -a 0x401000 -o C:\dumps -c C:\baseline\clean.hashes
  pd.exe -p "chrome[.]exe"
  pd.exe -p ".*chrome.*" -ni -nc
  pd.exe -pid 419 -reexec -o C:\dumps
  pd.exe -db add C:\baseline -nr -t 4
  pd.exe -closemon -o C:\dumps

Images/reconstructions above 256 MiB are rejected. Captures are live, non-atomic,
and may contain sensitive data; keep output private. See README.md for usage
examples and command-line options.
)help", stdout);
		return 0;
	}
	
	// Sanity check on flags
	if( (int) flagPidDump + (int) flagProcessNameDump + (int) flagSystemDump +
		(int) flagDB_gen + (int) flagDB_genQuick + (int) flagDB_add + (int) flagDB_clean + (int) flagDumpCloses > 1 )
	{
		// Only one of these at a time
		fprintf(stderr,"Error. Only one process dump or hash database command should be issued per execution.\n");
		exit(0);
	}

	if( flagAddressDump && !flagPidDump )
	{
		// Only one of these at a time
		fprintf(stderr,"Error. Dumping a specific address only works with the -pid flag to specify the process.\n");
		exit(0);
	}


	// Warn if the process was not run as administrator
	HANDLE h_Process = GetCurrentProcess();
	if( !is_elevated(h_Process) )
	{
		printf("WARNING: This tool should be run with administrator rights for best results.\n\n");
	}

	// Request maximum thread token privileges
	if( !get_privileges(h_Process) )
	{
		printf("WARNING: Failed to adjust token privileges. This may result in not being able to access some processes due to insufficient privileges.\n\n");
	}

	// Warn if running in 32 bit mode on a 64 bit OS
	if( is_win64() && sizeof(void*) == 4 )
	{
		printf("WARNING: To properly access all processes on a 64 bit Windows version, the 64 bit version of this tool should be used. Currently Process Dump is running as a 32bit process under a 64bit operating system.\n\n");
	}

	

	pe_hash_database* db = new pe_hash_database(clean_database, ep_database, epshort_database);


	if( flagDB_clean )
	{
		db->clear_database();
		printf("Cleared the clean hash database.\n");
		db->save();
	}else if( flagDB_add )
	{
		// Add the specified folder
		if( flagRecursion )
			printf("Adding all files in folder '%s' recursively to clean hash and entrypoint databases...\n", add_directory);
		else
			printf("Adding all files in folder '%s' to clean hash and entrypoint database...\n", add_directory);

		int count_before = db->count();
		db->add_folder(add_directory, L"*", flagRecursion, options.NumberOfThreads);
		printf("Added %i new hashes to the database. It now has %i hashes.\n", db->count() - count_before, db->count());
		db->save();
	}else if( flagDB_remove )
	{
		// Remove the specified folder
		if( flagRecursion )
			printf("Removing all files in folder '%s' recursively from the clean hash database...\n", add_directory);
		else
			printf("Removing all files in folder '%s' from the clean hash database...\n", add_directory);
		
		int count_before = db->count();
		db->remove_folder(add_directory, L"*", flagRecursion);
		printf("Removed %i hashes from the database. It now has %i hashes.\n", count_before - db->count(), db->count());
		db->save();
	}else if( flagDB_gen )
	{
		printf("Generating full clean database. This can take up to 30 minutes depending on the system.\n");

		// Add all the running processes to the clean hash database
		int count_before = db->count();
		printf("Adding modules from all running processes to clean hash database...\n");
		add_system_hashes( db, &options );
		printf("...added %i new hashes from running processes.\n", db->count() - count_before);
		db->save();

		// Add a bunch of folders to the database
		count_before = db->count();
		printf("Adding files in %%WINDIR%% to clean hash database...\n");
		db->add_folder("%WINDIR%", L"*", true, options.NumberOfThreads);
		printf("...added %i new hashes from %%WINDIR%%.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in %%USERPROFILE%% to clean hash database...\n");
		db->add_folder("%USERPROFILE%", L"*", true, options.NumberOfThreads);
		printf("...added %i new hashes from %%USERPROFILE%%.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in 'C:\\Program Files\\' to clean hash database...\n");
		db->add_folder("C:\\Program Files\\", L"*", true, options.NumberOfThreads);
		printf("...added %i new hashes from 'C:\\Program Files\\'.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in C:\\Program Files (x86)\\ to clean hash database...\n");
		db->add_folder("C:\\Program Files (x86)\\", L"*", true, options.NumberOfThreads);
		printf("...added %i new hashes from 'C:\\Program Files (x86)\\'.\n", db->count() - count_before);
		db->save();

		printf("\nFinished. The clean hash  database now has %i hashes.\n", db->count());
	}else if( flagDB_genQuick )
	{
		// Add all the running processes to the clean hash database
		int count_before = db->count();
		printf("Adding modules from all running processes to clean hash database...\n");
		add_system_hashes( db, &options );
		printf("...added %i new hashes from running processes.\n", db->count() - count_before);
		db->save();

		printf("\nFinished. The clean hash database now has %i hashes.\n", db->count());
	}

	// Clear the database if we set the flag to not use the database
	if( flagDB_ignore )
	{
		db->clear_database();
		printf("Ignoring the clean hash database for this execution.\n");
	}

	// Now process the dumping commands
	if( flagPidDump )
	{
		// Dump the specified PID
		dump_process* dumper = new dump_process( pid, db,  &options, false );

		if( flagAddressDump )
		{
			dumper->dump_region( address );
		}
		else
		{
			dumper->dump_all();
		}
		delete dumper;
	}
	else if( flagProcessNameDump )
	{
		// Dump the specified regex process name(s)

		// First gather the process matches
		DynArray<process_description*> matches;
		int count = process_find( processNameFilter, &matches );

		if( count > 1 )
		{
			// As the user if we should really dump all the found processes
			printf("\n\nPID\tProcess Name\n");
			for( int i = 0; i < count; i++ )
			{
				printf("0x%x\t%s\n", matches[i]->pid, matches[i]->process_name);
			}

			printf("\n\nAre you sure all of these processes should be dumped? (y/n): ");

			char* answer = new char[10];
			fgets( answer, 10, stdin );
			if( answer[0] != 'y' )
			{
				delete[] answer;
				exit(0);
			}
			delete[] answer;
		}

		// Loop through dumping the matching processes. Don't double-dump
		// modules with the same hash -- only dump them the first time
		// they are seen.
		unordered_set<unsigned __int64> new_hashes;
		for( int i = 0; i < count; i++ )
		{
			// Process this process
			dump_process* dumper = new dump_process( matches[i]->pid, db, &options, false );

			dumper->dump_all();

			// Exclude these hashes from the next dumps
			dumper->get_all_hashes( &new_hashes, NULL, NULL );
			db->add_hashes( new_hashes );
			new_hashes.clear();

			delete dumper;
		}
	}
	else if( flagSystemDump )
	{
		// Dump all processes running on the machine right now
		dump_system( db,  &options );
	}
	else if( flagPidDump )
	{
		// Dump the specified process
		dump_process* dumper = new dump_process( pid, db,  &options, false );
		dumper->dump_all();
		delete dumper;
	}
	else if (flagDumpCloses)
	{
		// Run in monitoring mode to dump all processes on close

		// Register the quit handler (CTRL-C to stop)
		if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
		{
			// unable to install handler... 
			// display message to the user
			printf("WARNING: Unable to install keyboard handler. This means that process dump will not be able to close or cleanup properly.\n");
		}

		// Start the hook monitor
		close_watcher* watcher = new close_watcher(db, &options);
		watcher->start_monitor();

		printf("------> Note: You may cleanly quit at any time by pressing CTRL-C. <------\n");

		// Wait until the user requests a close (by CTRL-C)
		while (!ConsoleRequestingClose)
		{
			Sleep(100);
		}
		
		// Cleanup properly
		printf("Cleaning up process terminate hooks cleanly...\n");
		watcher->stop_monitor();
		delete watcher;
	}

	printf("Finished running.\n");

	return 0;
}
catch (const std::exception& error)
{
	fprintf(stderr, "ERROR: Process Dump failed: %s.\n", error.what());
	return 1;
}
