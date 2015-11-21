#include "StdAfx.h"
#include "simple.h"


DWORD process_find(string match_regex, DynArray<process_description*>* result)
{
	PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if( snapshot != INVALID_HANDLE_VALUE )
	{
		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				char* process_name = new char[wcslen(entry.szExeFile)+1];
				sprintf( process_name, "%S", entry.szExeFile );
			
				string name (process_name);
				try
				{
					regex reg (match_regex);
					if( regex_match( name, reg ) )
					{  
						// Record this as a matching process
						result->Add( new process_description( process_name, entry.th32ProcessID ) );
					}
				}
				catch( std::tr1::regex_error e )
				{
					fprintf( stderr, "ERROR: Invalid regex expression for matching process names." );
					return 0;
				}


			}
		}

		CloseHandle(snapshot);
	}
	return result->GetSize();
}

string ExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA( NULL, buffer, MAX_PATH );
    string::size_type pos = string( buffer ).find_last_of( "\\/" );
    return string( buffer ).substr( 0, pos);
}

void PrintLastError(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 

	fwprintf(stderr,(LPCTSTR) lpDisplayBuf );

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}