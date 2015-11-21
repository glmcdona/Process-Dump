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

void add_system_hashes( pe_hash_database* db, PD_OPTIONS options )
{
	// Add clean hashes from all processes on the system right now

	// Build a list of the hashes from all processes
	DynArray<unsigned __int64> new_hashes;

	PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if( snapshot != INVALID_HANDLE_VALUE )
	{
		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				// Process this process
				printf("...adding hashes from process %S\n", entry.szExeFile);
				options.ImportRec = false; // Force no import reconstruciton
				dump_process* dumper = new dump_process( entry.th32ProcessID, db, options );
				dumper->get_all_hashes( &new_hashes );
				delete dumper;
			}
		}

		CloseHandle(snapshot);
	}
	
	// Add all these hashes to the database
	db->add_hashes( new_hashes );
}


void dump_system( pe_hash_database* db,  PD_OPTIONS options )
{
	// Dump all the modules on the system right now, but don't dump modules more than once
	DynArray<unsigned __int64> new_hashes;


	PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if( snapshot != INVALID_HANDLE_VALUE )
	{
		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				// Process this process
				dump_process* dumper = new dump_process( entry.th32ProcessID, db,  options );

				dumper->dump_all();

				// Exclude these hashes from the next dumps
				dumper->get_all_hashes( &new_hashes );
				db->add_hashes( new_hashes );
				new_hashes.Clear();

				delete dumper;
			}
		}

		CloseHandle(snapshot);
	}
	
	// Add all these hashes to the database
	db->add_hashes( new_hashes );
}


bool global_flag_verbose = false;

int _tmain(int argc, _TCHAR* argv[])
{

	get_privileges( GetCurrentProcess() );

	// Process the flags	
	WCHAR* filter = NULL;
	char* processNameFilter = NULL;
	bool flagHelp = false;
	bool flagHeader = true;
	bool flagPidDump = false;
	bool flagProcessNameDump = false;
	bool flagSystemDump = false;
	bool flagAddressDump = false;

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
					fprintf(stderr,"Failed to parse -pid argument. It must be preceeded by a number:\n\teg. 'pd -pid 0x10A'\n");
					exit(0);
				}

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -pid argument. It must be preceeded by a number:\n\teg. 'pd -pid 0x10A'\n");
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
					fprintf(stderr,"Failed to parse -a address argument. It must be preceeded by a number:\n\teg. 'pd -a 0x401000 -pid 0x10A'\n");
					exit(0);
				}

				// Skip next argument
				i++;
			}
			else
			{
				fprintf(stderr,"Failed to parse -pid argument. It must be preceeded by a number:\n\teg. 'pd -pid 0x10A'\n");
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
				fprintf(stderr,"Failed to parse -p argument. It must be preceeded by a regex match statement:\n\teg. 'pd -p chrome.exe'\n");
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
						add_directory = new char[wcslen(argv[i+2]) + 1];
						sprintf( add_directory, "%S", argv[i+2] );

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
						fprintf(stderr,"Failed to parse '-db add' argument. It must be preceeded by a directory to add:\n\teg. 'pd -db add C:\\Windows\\'\n");
						exit(0);
					}
					i+=1;
				}else if( lstrcmp(argv[i+1],L"remove") == 0 || lstrcmp(argv[i+1],L"rem") == 0 )
				{
					// Has yet another argument to specify the directory to remove
					if(  i + 2 < argc )
					{
						// Extract the directory name to remove
						add_directory = new char[wcslen(argv[i+2]) + 1];
						sprintf( add_directory, "%S", argv[i+2] );

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
						fprintf(stderr,"Failed to parse '-db remove' argument. It must be preceeded by a directory to remove:\n\teg. 'pd -db add C:\\Windows\\'\n");
						exit(0);
					}
					i+=1;
				}


				i+=1;
			}else{
				fprintf(stderr,"Failed to parse -db argument. It must be preceeded by a command:\n\teg. 'pd -db genquick'\n");
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
		printf("Process Dump v1.5\n");
		printf("  Copyright © 2015, Geoff McDonald\n");
		printf("  http://www.split-code.com/\n");
		printf("  https://github.com/glmcdona/Process-Dump\n\n");
	}

	if( flagHelp )
	{
		// Print help page
		printf("Process Dump (pd.exe) is a tool used to dump both 32 and 64 bit executable modules back to disk from memory within a process address space. This tool is able to find and dump hidden modules, and it uses a clean hash database to exclude dumping of known clean files. This tool uses an aggressive import reconstruction approach that links all DWORD/QWORDs that point to an export in the process to the corresponding export function.\n\n");
		printf("Before first usage of this tool, when on the clean workstation the clean exclusing hash database can be generated by either:\n");
		printf("\tpd -db gen\n");
		printf("\tpd -db genquick\n\n");
		printf("Example Usage:\n");
		printf("\tpd -system\n");
		printf("\tpd -p chrome.exe\n");
		printf("\tpd -p \"(?i).*chrome.*\"\n");
		printf("\tpd -pid 419\n");
		printf("\tpd -pid 0x1a3\n");
		printf("\tpd -pid 0x1a3 -a 0x401000\n\n");

		printf("Options:\n");
		printf("\t-pid <pid>\tDumps all modules not matching the clean hash database\n\t\t\tfrom the specified pid into the current working\n\t\t\tdirectory. Use a '0x' prefix to specify a hex PID.\n\n");
		printf("\t-p <regex>\tDumps all modules not matching the clean hash database\n\t\t\tfrom the process name found to match the filter into\n\t\t\tspecified pid into the current working directory.\n\n");
		printf("\t-system\t\tDumps all modules not matching the clean hash database\n\t\t\tfrom all accessible processes into the working\n\t\t\tdirectory.\n\n" );
		printf("\t-g\t\tForces generation of PE headers from scratch, ignoring existing headers.\n\n");
		printf("\t-v\t\tVerbose.\n\n");
		printf("\t-nh\t\tNo header is printed in the output.\n\n");
		printf("\t-nr\t\tDisable recursion on hash database directory add or\n\t\t\tremove commands.\n\n");
		printf("\t-ni\t\tDisable import reconstruction.\n\n");
		printf("\t-db gen\t\tAutomatically processes a few common folders as well as\n\t\t\tall the currently running processes and adds the found\n\t\t\tmodule hashes to the clean hash database. It will add\n\t\t\tall files recursively in: \n\t\t\t\t%%WINDIR%% \n\t\t\t\t%%HOMEPATH%% \n\t\t\t\tC:\\Program Files\\ \n\t\t\t\tC:\\Program Files (x86)\\ \n\t\t\tAs well as all modules in all running processes \n\n");
		printf("\t-db genquick\tAdds the hashes from all modules in all processes to\n\t\t\tthe clean hash database. Run this on a clean system.\n\n");
		printf("\t-db add <dir>\tAdds all the files in the specified directory\n\t\t\trecursively to the clean hash database. \n\n");
		printf("\t-db rem <dir>\tRemoves all the files in the specified directory\n\t\t\trecursively from the clean hash database. \n\n");
		printf("\t-db clean\tClears the clean hash database.\n\n");
		printf("\t-db ignore\tIgnores the clean hash database when dumping a process\n\t\t\tthis time.  All modules will be dumped even if a match\n\t\t\tis found.\n\n");
	}
	
	// Sanity check on flags
	if( (int) flagPidDump + (int) flagProcessNameDump + (int) flagSystemDump +
		(int) flagDB_gen + (int) flagDB_genQuick + (int) flagDB_add + (int) flagDB_clean > 1 )
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

	

	pe_hash_database* db = new pe_hash_database("clean.hashes");


	if( flagDB_clean )
	{
		db->clear_database();
		printf("Cleared the clean hash database.\n");
		db->save();
	}else if( flagDB_add )
	{
		// Add the specified folder
		if( flagRecursion )
			printf("Adding all files in folder '%s' recursively to clean hash database...\n", add_directory);
		else
			printf("Adding all files in folder '%s' to clean hash database...\n", add_directory);

		int count_before = db->count();
		db->add_folder(add_directory, L"*", flagRecursion);
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
		// Add all the running processes to the clean hash database
		int count_before = db->count();
		printf("Adding modules from all running processes to clean hash database...\n");
		add_system_hashes( db, options );
		printf("...added %i new hashes from running processes.\n", db->count() - count_before);
		db->save();

		// Add a bunch of folders to the database
		count_before = db->count();
		printf("Adding files in %WINDIR% to clean hash database...\n");
		db->add_folder("%WINDIR%", L"*", true);
		printf("...added %i new hashes from %WINDIR%.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in %HOMEPATH% to clean hash database...\n");
		db->add_folder("%HOMEPATH%", L"*", true);
		printf("...added %i new hashes from %HOMEPATH %.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in 'C:\\Program Files\\' to clean hash database...\n");
		db->add_folder("C:\\Program Files\\", L"*", true);
		printf("...added %i new hashes from 'C:\\Program Files\\'.\n", db->count() - count_before);
		db->save();

		count_before = db->count();
		printf("Adding files in C:\\Program Files (x86)\\ to clean hash database...\n");
		db->add_folder("C:\\Program Files (x86)\\", L"*", true);
		printf("...added %i new hashes from 'C:\\Program Files (x86)\\'.\n", db->count() - count_before);
		db->save();

		printf("\nFinished. The clean hash  database now has %i hashes.\n", db->count());
	}else if( flagDB_genQuick )
	{
		// Add all the running processes to the clean hash database
		int count_before = db->count();
		printf("Adding modules from all running processes to clean hash database...\n");
		add_system_hashes( db, options );
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
		dump_process* dumper = new dump_process( pid, db,  options );

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
		DynArray<unsigned __int64> new_hashes;
		for( int i = 0; i < count; i++ )
		{
			// Process this process
			dump_process* dumper = new dump_process( matches[i]->pid, db, options );

			dumper->dump_all();

			// Exclude these hashes from the next dumps
			dumper->get_all_hashes( &new_hashes );
			db->add_hashes( new_hashes );
			new_hashes.Clear();

			delete dumper;
		}
	}
	else if( flagSystemDump )
	{
		// Dump all processes running on the machine right now
		dump_system( db,  options );
	}
	else if( flagPidDump )
	{
		// Dump the specified process
		dump_process* dumper = new dump_process( pid, db,  options );
		dumper->dump_all();
		delete dumper;
	}

	return 0;
}


