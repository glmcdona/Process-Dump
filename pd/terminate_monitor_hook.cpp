#include "stdafx.h"
#include "terminate_monitor_hook.h"


bool terminate_monitor_hook::add_redirect(unsigned __int64 target_address)
{
	SIZE_T num_written = 0;
	unsigned char inject[0x20];

	if (!_is64)
	{
		// Redirect code that is the same for x86 and AMD64
		unsigned char data[] = {
			// call +0
			0xE8, 0x00, 0x00, 0x00, 0x00,

			// pop eax
			0x58,

			// add eax, 0x6
			0x83, 0xC0, 0x06,

			// jmp [eax]
			0xFF, 0x20,

			// <target>
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <target_address>
		};

		*(unsigned __int64*)(data + 11) = (unsigned __int64)(target_address);
		memcpy(inject, data, sizeof(data));
	}
	else
	{
		// 64-bit code
		unsigned char data[] = {
			// call +0
			0xE8, 0x00, 0x00, 0x00, 0x00,

			// pop rax
			0x58,

			// add rax, 0x7
			0x48, 0x83, 0xC0, 0x07,

			// jmp [rax]
			0xFF, 0x20,

			// <target>
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <target_address>
		};

		*(unsigned __int64*)(data + 12) = (unsigned __int64)(target_address);
		memcpy(inject, data, sizeof(data));
	}
	
	
	// Write the redirect
	bool success = WriteProcessMemory(_ph, (LPVOID)  _address_terminate, inject, sizeof(inject), &num_written);
	if (success && sizeof(inject) == num_written)
	{
		return true;
	}
	
	return false;
}

bool terminate_monitor_hook::unhock_terminate()
{
	if (_address_terminate != NULL && _original_hook_bytes != NULL)
	{
		// Write the original code back
		SIZE_T num_written = 0;
		bool success = WriteProcessMemory(_ph, (LPVOID) _address_terminate, _original_hook_bytes, sizeof(_original_hook_bytes), &num_written);

		// Handle any stuck threads that are waiting to terminate
		if (is_terminate_waiting())
		{
			resume_terminate();

			// no need to cleanup hook allocation since process is terminating
		}
		else
		{
			// Remove our hook allocation
			if (!_process_is_terminating && _hook_address != NULL)
				VirtualFreeEx(_ph, (LPVOID)_hook_address, 0, MEM_RELEASE);
		}

		_hook_address = NULL;
		_address_is_waiting = NULL;
		_address_thread_id = NULL;
	}
	
	return true;
}

unsigned __int64 terminate_monitor_hook::get_address(char * library, char * procedure_name)
{
	return 0;
}

bool terminate_monitor_hook::get_terminate_orig_code(unsigned char * out_buffer, int length)
{
	return false;
}

bool terminate_monitor_hook::hook_terminate(export_list* exports)
{
	// Allocate space in the remote process for the hook
	if (_hook_address == NULL)
	{
		_hook_address = (unsigned __int64)VirtualAllocEx(_ph, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (_hook_address > 0)
		{
			// Lookup the function addresses in the target process
			_address_terminate = exports->find_export("ntdll.dll", "NtTerminateProcess", _is64);
			unsigned __int64 address_getthread = exports->find_export("kernel32.dll", "GetCurrentThread", _is64);
			unsigned __int64 address_getthreadid = exports->find_export("kernel32.dll", "GetCurrentThreadId", _is64);
			unsigned __int64 address_suspendthread = exports->find_export("kernel32.dll", "SuspendThread", _is64);

			if (_address_terminate == NULL || address_getthread == NULL || address_suspendthread == NULL || address_getthreadid == NULL)
			{
				if (_options->Verbose)
				{
					fprintf(stderr, "WARNING: Failed to find library exports used in hooking ZwTerminateProcess. PID: 0x%x, 64bit mode: %i\r\n", _pid, (int)_is64);
					fprintf(stderr, "WARNING: ntdll.dll::NtTerminateProcess = 0x%llX\r\n", _address_terminate);
					fprintf(stderr, "WARNING: kernel32.dll::GetCurrentThreadId = 0x%llX\r\n", address_getthreadid);
					fprintf(stderr, "WARNING: kernel32.dll::GetCurrentThread = 0x%llX\r\n", address_getthread);
					fprintf(stderr, "WARNING: kernel32.dll::SuspendThread = 0x%llX\r\n", address_suspendthread);
				}
			}
			else
			{
				// Found the addresses for our hook code

				// Modify the NtTerminateProcess region to add WRITE privileges
				//VirtualQueryEx( _ph, _address_terminate, )
				DWORD old_protection = 0;
				bool changed = VirtualProtectEx(_ph, (LPVOID) _address_terminate, 0x1000, PAGE_EXECUTE_READWRITE, &old_protection);

				if (changed)
				{
					// Read the original code
					SIZE_T num_read = 0;
					bool success = ReadProcessMemory(_ph, (LPCVOID)(_address_terminate), (void*)(_original_hook_bytes), sizeof(_original_hook_bytes), &num_read);

					if (num_read == sizeof(_original_hook_bytes) && success)
					{
						unsigned char* hook_code = NULL;
						int hook_code_length = 0;

						if (!_is64)
						{
							// Windows 10, wow64 version of ZwTerminateProcess
							// ntdll.dll -> ZwTerminateProcess()
							// B8 29 00 00 00          mov     eax, 29h; NtTerminateProcess
							// 33 C9                   xor     ecx, ecx
							// 8D 54 24 04             lea     edx, [esp + arg_0]
							// 64 FF 15 C0 00 00 00    call    large dword ptr fs : 0C0h
							// 83 C4 04                add     esp, 4
							// C2 08 00                retn    8

							// ------ Hook (x86/x64 independent code) -------
							//	is_waiting: (00)
							//		dq 0
							//	thread_id: (08)
							//		dq 0
							//	GetCurrentThread: (10)
							//		dq 0
							//	GetCurrentThreadId: (18)
							//		dq 0
							//	SuspendThread: (20)
							//		dq 0
							//	TerminateProcess: (28)
							//		dq 0
							//  _hook_original_code_length: (30)
							//		dq 0
							//	_hook_original_code: (38)
							//		dq 0
							//		dq 0
							//		dq 0
							//		dq 0
							//	hook:
							//		push ebx
							//		push ecx
							//		push esi
							//		push edi
							//		
							//		call 0
							//		pop ebx
							//		sub ebx, 0x5d  // 5 + 0x58 = 0x5d
							//
							//		push eax
							//		mov eax, [ebx+0x10] // GetCurrentThreadId
							//		call eax
							//		mov [ebx+0x08], eax // thread_id
							//		mov [ebx], 1	// is_waiting
							//		push eax
							//		mov eax, [ebx+0x18] // SuspendThread
							//		call eax
							//
							//		mov ecx, [ebx+0x28] // _hook_original_code_length
							//		mov esi, ebx
							//		add esi, 0x30 // _hook_original_code
							//		mov edi, [ebx+0x20] // TerminateProcess
							//		rep movsb
							//		
							//		mov eax, [ebx+0x20] // TerminateProcess
							//      pop edi
							//      pop esi
							//      pop ecx
							//      pop ebx
							//
							//		jmp eax

							unsigned char code[] = {
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <is_waiting>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <thread_id>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <GetCurrentThreadId>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <GetCurrentThread>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <SuspendThread>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <TerminateProcess>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, // <_hook_original_code_length>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code>
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 8
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 16
								0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 24


								//		push ebx
								//		push ecx
								//		push esi
								//		push edi
								0x53, 0x51, 0x56, 0x57,

								//		call 0
								//		pop ebx
								//		sub ebx, 0x61  // 5 + 4 + 0x58 = 0x61
								0xE8, 0x00, 0x00, 0x00, 0x00,
								0x5B,
								0x83, 0xEB, 0x61,

								//		mov eax, [ebx+0x10] // GetCurrentThreadId
								//		call eax
								//		mov [ebx+0x08], eax // thread_id
								0x8B, 0x43, 0x10,
								0xFF, 0xD0,
								0x89, 0x43, 0x08,
								
								//		mov eax, [ebx+0x18] // GetCurrentThread
								//		call eax
								0x8B, 0x43, 0x18,
								0xFF, 0xD0,

								//		mov [ebx], 1	// is_waiting
								//		push eax
								//		mov eax, [ebx+0x20] // SuspendThread
								//		call eax
								0xC7, 0x03,	0x01, 0x00, 0x00, 0x00,
								0x50,
								0x8B, 0x43, 0x20,
								0xFF, 0xD0,

								//		mov ecx, [ebx+0x30] // _hook_original_code_length
								//		mov esi, ebx
								//		add esi, 0x38 // _hook_original_code
								//		mov edi, [ebx+0x28] // TerminateProcess
								//		cld
								//		rep movsb [edi], [esi]
								0x8B, 0x4B, 0x30,
								0x89, 0xDE,
								0x83, 0xC6, 0x38,
								0x8B, 0x7B, 0x28,
								0xFC,
								0xF3, 0xA4,

								//		mov eax, [ebx+0x28] // TerminateProcess
								0x8B, 0x43, 0x28,

								//      pop edi
								//      pop esi
								//      pop ecx
								//      pop ebx
								0x5F, 0x5E, 0x59, 0x5B,

								//0xCC,

								//		jmp eax
								0xFF, 0xE0

							};

							// Fill out the variables in the code
							//	is_waiting: (00)
							//		dq 0
							//	thread_id: (08)
							//		dq 0
							//	GetCurrentThreadId: (10)
							//		dq 0
							//	SuspendThread: (18)
							//		dq 0
							//	TerminateProcess: (20)
							//		dq 0
							//  _hook_original_code_length: (28)
							//		dq 0
							//	_hook_original_code: (30)
							//		dq 0
							//		dq 0
							hook_code = new unsigned char[sizeof(code)];
							hook_code_length = sizeof(code);
							*((unsigned __int64*)(code + 0x10)) = address_getthreadid; // GetCurrentThreadId
							*((unsigned __int64*)(code + 0x18)) = address_getthread; // GetCurrentThread
							*((unsigned __int64*)(code + 0x20)) = address_suspendthread; // SuspendThread
							*((unsigned __int64*)(code + 0x28)) = _address_terminate; // <address_terminate>
							*((unsigned __int64*)(code + 0x30)) = 32; // _hook_original_code_length
							memcpy(code + 0x38, _original_hook_bytes, 32); // _hook_original_code

							// Copy to use this code as our hook code
							memcpy(hook_code, code, sizeof(code));
						}
						else
						{
							// total size should be 0x47 (compiled code) + 0x58 (variables) = 0x9f
							unsigned char code64[] = {
								/*  00  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <is_waiting>
								/*  08  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <thread_id>
								/*  10  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <GetCurrentThreadId>
								/*  18  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <GetCurrentThread>
								/*  20  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <SuspendThread>
								/*  28  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <TerminateProcess>
								/*  30  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, // <_hook_original_code_length>
								/*  38  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code>
								/*  40  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 8
								/*  48  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 16
								/*  50  */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <_hook_original_code> + 24


								//		push rbx
								//		push rcx
								//		push rsi
								//		push rdi
								0x53, 0x51, 0x56, 0x57,

								//		call +0
								//		pop rbx
								//		sub rbx, 0x61  // 5 + 4 + 0x58 = 0x61
								0xE8, 0x00, 0x00, 0x00, 0x00,
								0x5B,
								0x48, 0x83, 0xEB, 0x61,

								//		mov rax, [rbx+0x10] // GetCurrentThreadId
								//		sub rsp, 0x20
								//		call rax
								//		add rsp, 0x20
								//		mov [rbx+0x08], rax // thread_id
								0x48, 0x8B, 0x43, 0x10,
								0x48, 0x83, 0xEC, 0x20,
								0xFF, 0xD0,
								0x48, 0x83, 0xC4, 0x20,
								0x48, 0x89, 0x43, 0x08,

								//		mov rax, [rbx+0x18] // GetCurrentThread
								//		sub rsp, 0x20
								//		call rax
								//		add rsp, 0x20
								0x48, 0x8B, 0x43, 0x18,
								0x48, 0x83, 0xEC, 0x20,
								0xFF, 0xD0,
								0x48, 0x83, 0xC4, 0x20,

								//		mov QWORD [rbx], 1	// is_waiting
								// 4889C1  mov rcx, rax		; threadid
								//		mov rax, [rbx+0x20] // SuspendThread
								//      sub rsp, 0x20
								//		call rax
								//		add rsp, 0x20
								0x48, 0xC7, 0x03, 0x01, 0x00, 0x00, 0x00,
								0x48, 0x89, 0xC1,
								0x48, 0x8B, 0x43, 0x20,
								0x48, 0x83, 0xEC, 0x20,
								0xFF, 0xD0,
								0x48, 0x83, 0xC4, 0x20,

								//		mov rcx, [rbx+0x30] // _hook_original_code_length
								//		mov rsi, rbx
								//		add rsi, 0x38 // _hook_original_code
								//		mov rdi, [rbx+0x28] // TerminateProcess
								//		cld
								//		rep movsb [rdi], [rsi]
								0x48, 0x8B, 0x4B, 0x30,
								0x48, 0x89, 0xDE,
								0x48, 0x83, 0xC6, 0x38,
								0x48, 0x8B, 0x7B, 0x28,
								0xFC,
								0xF3, 0xA4,

								//		mov rax, [rbx+0x28] // TerminateProcess
								0x48, 0x8B, 0x43, 0x28,

								//      pop rdi
								//      pop rsi
								//      pop rcx
								//      pop rbx
								0x5F, 0x5E, 0x59, 0x5B,

								//0xCC,

								//		jmp rax
								0xFF, 0xE0

							};

							// Fill out the variables in the code
							//	is_waiting: (00)
							//		dq 0
							//	thread_id: (08)
							//		dq 0
							//	GetCurrentThreadId: (10)
							//		dq 0
							//	SuspendThread: (18)
							//		dq 0
							//	TerminateProcess: (20)
							//		dq 0
							//  _hook_original_code_length: (28)
							//		dq 0
							//	_hook_original_code: (30)
							//		dq 0
							//		dq 0
							hook_code = new unsigned char[sizeof(code64)];
							hook_code_length = sizeof(code64);
							*((unsigned __int64*)(code64 + 0x10)) = address_getthreadid; // GetCurrentThreadId
							*((unsigned __int64*)(code64 + 0x18)) = address_getthread; // GetCurrentThread
							*((unsigned __int64*)(code64 + 0x20)) = address_suspendthread; // SuspendThread
							*((unsigned __int64*)(code64 + 0x28)) = _address_terminate; // <address_terminate>
							*((unsigned __int64*)(code64 + 0x30)) = 32; // _hook_original_code_length
							memcpy(code64 + 0x38, _original_hook_bytes, 32); // _hook_original_code (_original_hook_bytes is not our hook!)

							// Copy to use this code as our hook code
							memcpy(hook_code, code64, sizeof(code64));
						}

						// Inject the hook code
						SIZE_T num_written = 0;
						bool success = WriteProcessMemory(_ph, (LPVOID)_hook_address, hook_code, hook_code_length, &num_written);
						if (hook_code != NULL)
							delete[]hook_code;

						if (success && num_written == hook_code_length)
						{
							// Set the variable addresses in the remote process
							_address_is_waiting = _hook_address;
							_address_thread_id = _hook_address + 8;

							// Redirect ZwTerminateProcess
							return add_redirect(_hook_address + 0x58);
						}
						else
						{
							PrintLastError(L"Failed to write NtTerminateProcess hook code.");
						}
					}
				}
			}
		}
		else
		{
			if( _options->Verbose )
				PrintLastError(L"Failed to allocate space for NtTerminateProcess hook.");
		}

		return false; // Failed to hook
	}
	return true; // Already hooked
}

bool terminate_monitor_hook::is_terminate_waiting()
{
	if (_hook_address != NULL && _address_is_waiting != NULL )
	{
		unsigned __int64 value = 0;
		SIZE_T num_read = 0;
		bool success = ReadProcessMemory(_ph, (LPCVOID) _address_is_waiting, &value, sizeof(value), &num_read);
		if (success && num_read == sizeof(value))
		{
			return value == 1;
		}
	}
	return false;
}

void terminate_monitor_hook::resume_terminate()
{
	if (is_terminate_waiting() && _address_thread_id != NULL)
	{
		DWORD tid = 0;
		if ( read_memory(_ph, _address_thread_id, &tid) && tid != 0 )
		{
			// Set it's state to no longer waiting to terminate
			write_memory(_ph, _address_is_waiting, (unsigned __int64)0);

			// Resume this thread in the target process
			HANDLE th = OpenThread(THREAD_SUSPEND_RESUME, false, tid);
			DWORD result = ResumeThread(th);

			_process_is_terminating = true;
		}
	}
}

terminate_monitor_hook::terminate_monitor_hook(HANDLE ph, DWORD pid, bool is64, PD_OPTIONS* options)
{
	_ph = ph;
	_pid = pid;
	_is64 = is64;
	_hook_address = NULL;
	_address_is_waiting = NULL;
	_address_thread_id = NULL;
	_address_terminate = NULL;
	_process_is_terminating = false;
	_options = options;
}


terminate_monitor_hook::~terminate_monitor_hook()
{
	// Remove our redirect from this process if it is in place
	unhock_terminate();
}
