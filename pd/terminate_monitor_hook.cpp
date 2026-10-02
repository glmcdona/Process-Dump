#include "stdafx.h"
#include "terminate_monitor_hook.h"

namespace
{
	const SIZE_T hook_code_offset = 0x1000;
	const SIZE_T hook_allocation_size = 0x2000;

	struct hook_data
	{
		unsigned __int64 waiting;
		unsigned __int64 release_event;
		unsigned __int64 wait_for_event;
		unsigned __int64 terminate;
	};
	static_assert(sizeof(hook_data) == 32, "Hook data offsets must match both architectures");
}

bool terminate_monitor_hook::executable_address(unsigned __int64 address)
{
	if (address == 0 || address > UINTPTR_MAX)
		return false;
	MEMORY_BASIC_INFORMATION region = {};
	return VirtualQueryEx(_ph, (LPCVOID)address, &region, sizeof(region)) == sizeof(region) &&
		region.State == MEM_COMMIT && !(region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
		(region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
}

bool terminate_monitor_hook::restore_protection()
{
	if (!_protection_changed)
		return true;
	DWORD previous;
	if (!VirtualProtectEx(_ph, (LPVOID)_address_terminate, sizeof(_original_hook_bytes), _original_protection, &previous))
	{
		PrintLastError(L"Restoring terminate protection");
		return false;
	}
	_protection_changed = false;
	return true;
}

bool terminate_monitor_hook::add_redirect(unsigned __int64 target_address)
{
	unsigned char jump[14] = {};
	SIZE_T size;
	if (_is64)
	{
		jump[0] = 0xff;
		jump[1] = 0x25; // jmp qword ptr [rip]
		memcpy(jump + 6, &target_address, sizeof(target_address));
		size = sizeof(jump);
	}
	else
	{
		jump[0] = 0xe9;
		const DWORD relative = static_cast<DWORD>(target_address - _address_terminate - 5);
		memcpy(jump + 1, &relative, sizeof(relative));
		size = 5;
	}
	if (!VirtualProtectEx(_ph, (LPVOID)_address_terminate, sizeof(_original_hook_bytes),
		PAGE_EXECUTE_READWRITE, &_original_protection))
		return false;
	_protection_changed = true;
	_redirect_attempted = true;
	SIZE_T written = 0;
	const bool success = WriteProcessMemory(_ph, (LPVOID)_address_terminate, jump, size, &written) &&
		written == size && FlushInstructionCache(_ph, (LPCVOID)_address_terminate, size);
	const bool protected_again = restore_protection();
	return success && protected_again;
}

bool terminate_monitor_hook::close_unpublished_event()
{
	if (_remote_event == NULL)
		return true;
	HANDLE local_copy = NULL;
	if (!DuplicateHandle(_ph, _remote_event, GetCurrentProcess(), &local_copy, 0, FALSE,
		DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
	{
		PrintLastError(L"Closing unpublished remote event");
		return false;
	}
	CloseHandle(local_copy);
	_remote_event = NULL;
	return true;
}

bool terminate_monitor_hook::unhock_terminate()
{
	if (_redirect_attempted && _original_bytes_valid)
	{
		DWORD previous = 0;
		if (!VirtualProtectEx(_ph, (LPVOID)_address_terminate, sizeof(_original_hook_bytes), PAGE_EXECUTE_READWRITE, &previous))
		{
			PrintLastError(L"Making terminate code writable for restoration");
			return false;
		}
		_protection_changed = true;
		SIZE_T written = 0;
		const bool restored = WriteProcessMemory(_ph, (LPVOID)_address_terminate, _original_hook_bytes,
			sizeof(_original_hook_bytes), &written) && written == sizeof(_original_hook_bytes) &&
			FlushInstructionCache(_ph, (LPCVOID)_address_terminate, sizeof(_original_hook_bytes));
		const bool protected_again = restore_protection();
		if (!restored || !protected_again)
		{
			PrintLastError(L"Restoring terminate code");
			return false;
		}
		// Signal even if the callback has not started waiting yet. The private manual-reset
		// event avoids trusting a target-supplied thread ID and cannot lose an early wakeup.
		if (_release_event != NULL && !SetEvent(_release_event))
		{
			PrintLastError(L"Releasing terminate callback");
			return false;
		}
		// Published code and its remote event remain valid for any in-flight callback.
		// The operating system reclaims them when the monitored process exits.
		_remote_event = NULL;
	}
	else
	{
		if (!restore_protection() || !close_unpublished_event())
			return false;
		if (_hook_address != 0 && !VirtualFreeEx(_ph, (LPVOID)_hook_address, 0, MEM_RELEASE))
		{
			PrintLastError(L"Freeing unpublished terminate hook");
			return false;
		}
	}
	if (_release_event != NULL)
		CloseHandle(_release_event);
	_release_event = NULL;
	_hook_address = 0;
	_address_terminate = 0;
	_original_bytes_valid = false;
	_redirect_attempted = false;
	_installed = false;
	return true;
}

bool terminate_monitor_hook::hook_terminate(export_list* exports)
{
	if (_hook_address != 0)
		return _installed;
	if (exports == NULL)
	{
		fprintf(stderr, "ERROR: Cannot install terminate hook without exports.\n");
		return false;
	}
	_address_terminate = exports->find_export("ntdll.dll", "NtTerminateProcess", _is64);
	unsigned __int64 wait_address = exports->find_export("kernelbase.dll", "WaitForSingleObject", _is64);
	if (!executable_address(wait_address))
		wait_address = exports->find_export("kernel32.dll", "WaitForSingleObject", _is64);
	if (!executable_address(_address_terminate) || !executable_address(wait_address))
	{
		if (_options->Verbose)
			fprintf(stderr, "WARNING: Missing executable terminate/wait exports in PID 0x%x.\n", _pid);
		return false;
	}
	SIZE_T read = 0;
	if (!ReadProcessMemory(_ph, (LPCVOID)_address_terminate, _original_hook_bytes, sizeof(_original_hook_bytes), &read) ||
		read != sizeof(_original_hook_bytes))
	{
		PrintLastError(L"Reading original terminate code");
		return false;
	}
	_original_bytes_valid = true;
	_release_event = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (_release_event == NULL || !DuplicateHandle(GetCurrentProcess(), _release_event, _ph, &_remote_event,
		SYNCHRONIZE, FALSE, 0))
	{
		PrintLastError(L"Creating terminate release event");
		unhock_terminate();
		return false;
	}
	_hook_address = reinterpret_cast<uintptr_t>(VirtualAllocEx(_ph, NULL, hook_allocation_size,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (_hook_address == 0 || (!_is64 && (_hook_address + hook_allocation_size > MAXDWORD ||
		reinterpret_cast<uintptr_t>(_remote_event) > MAXDWORD)))
	{
		PrintLastError(L"Allocating terminate callback");
		unhock_terminate();
		return false;
	}

	// Preserve the original NtTerminateProcess arguments; only the host restores its entry
	// bytes. Data stays RW and the callback's separate page becomes RX before publication.
	unsigned char code32[] = {
		0x53, 0x51, 0x52,                         // push ebx, ecx, edx
		0xbb, 0, 0, 0, 0,                        // mov ebx, data
		0xc7, 0x03, 1, 0, 0, 0,                  // mov dword ptr [ebx], 1
		0x6a, 0xff, 0xff, 0x73, 0x08,            // push INFINITE; push [ebx+8]
		0xff, 0x53, 0x10,                        // call [ebx+16]
		0x8b, 0x43, 0x18,                        // mov eax, [ebx+24]
		0x5a, 0x59, 0x5b, 0xff, 0xe0             // restore arguments; jmp eax
	};
	unsigned char code64[] = {
		0x53, 0x51, 0x52,                         // push rbx, rcx, rdx (align stack)
		0x48, 0xbb, 0, 0, 0, 0, 0, 0, 0, 0,      // mov rbx, data
		0x48, 0xc7, 0x03, 1, 0, 0, 0,            // mov qword ptr [rbx], 1
		0x48, 0x8b, 0x4b, 0x08,                  // mov rcx, [rbx+8]
		0xba, 0xff, 0xff, 0xff, 0xff,             // mov edx, INFINITE
		0x48, 0x83, 0xec, 0x20,                  // allocate shadow space
		0xff, 0x53, 0x10,                        // call [rbx+16]
		0x48, 0x83, 0xc4, 0x20,
		0x48, 0x8b, 0x43, 0x18,                  // mov rax, [rbx+24]
		0x5a, 0x59, 0x5b, 0xff, 0xe0             // restore arguments; jmp rax
	};
	const DWORD address32 = static_cast<DWORD>(_hook_address);
	memcpy(code32 + 4, &address32, sizeof(address32));
	memcpy(code64 + 5, &_hook_address, sizeof(_hook_address));
	const unsigned char* code = _is64 ? code64 : code32;
	const SIZE_T code_size = _is64 ? sizeof(code64) : sizeof(code32);
	hook_data data = {0, reinterpret_cast<uintptr_t>(_remote_event), wait_address, _address_terminate};
	SIZE_T written = 0;
	DWORD old_protection = 0;
	if (!WriteProcessMemory(_ph, (LPVOID)_hook_address, &data, sizeof(data), &written) || written != sizeof(data) ||
		!WriteProcessMemory(_ph, (LPVOID)(_hook_address + hook_code_offset), code, code_size, &written) || written != code_size ||
		!VirtualProtectEx(_ph, (LPVOID)(_hook_address + hook_code_offset), 0x1000, PAGE_EXECUTE_READ, &old_protection) ||
		!FlushInstructionCache(_ph, (LPCVOID)(_hook_address + hook_code_offset), code_size) ||
		!add_redirect(_hook_address + hook_code_offset))
	{
		PrintLastError(L"Publishing terminate callback");
		unhock_terminate();
		return false;
	}
	_installed = true;
	return true;
}

bool terminate_monitor_hook::is_terminate_waiting()
{
	unsigned __int64 waiting = 0;
	return _installed && read_memory(_ph, _hook_address, &waiting) && waiting == 1;
}

bool terminate_monitor_hook::resume_terminate()
{
	return unhock_terminate();
}

terminate_monitor_hook::terminate_monitor_hook(HANDLE ph, DWORD pid, bool is64, PD_OPTIONS* options)
	: _ph(ph), _pid(pid), _is64(is64), _options(options)
{
}

terminate_monitor_hook::~terminate_monitor_hook()
{
	if (!unhock_terminate() && _release_event != NULL)
		CloseHandle(_release_event);
}
