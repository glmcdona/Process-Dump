#include "test_support.h"
#include <algorithm>

// Exercise the production hook state machine with mocked OS calls; no process is patched.
namespace hook_mock
{
	const uintptr_t allocation = 0x10000;
	const uintptr_t terminate_address = 0x20000;
	const uintptr_t wait_address = 0x30000;
	std::vector<unsigned char> memory;
	unsigned char original[32];
	unsigned char target[32];
	bool fail_read, fail_redirect, fail_event;
	int writes, frees, signals, remote_closes, local_closes;
	DWORD protection;

	void reset()
	{
		memory.assign(0x2000, 0);
		memset(original, 0x90, sizeof(original));
		memcpy(target, original, sizeof(target));
		fail_read = fail_redirect = fail_event = false;
		writes = frees = signals = remote_closes = local_closes = 0;
		protection = PAGE_EXECUTE_READ;
	}
	SIZE_T WINAPI query(HANDLE, LPCVOID, PMEMORY_BASIC_INFORMATION region, SIZE_T size)
	{
		memset(region, 0, sizeof(*region));
		region->State = MEM_COMMIT;
		region->Protect = PAGE_EXECUTE_READ;
		region->RegionSize = 0x1000;
		return size;
	}
	BOOL WINAPI read(HANDLE, LPCVOID address, LPVOID output, SIZE_T size, SIZE_T* count)
	{
		*count = 0;
		if (fail_read)
		{
			SetLastError(ERROR_PARTIAL_COPY);
			return FALSE;
		}
		if (reinterpret_cast<uintptr_t>(address) == terminate_address && size == sizeof(target))
		{
			memcpy(output, target, size);
			*count = size;
			return TRUE;
		}
		return FALSE;
	}
	BOOL WINAPI write(HANDLE, LPVOID address, LPCVOID input, SIZE_T size, SIZE_T* count)
	{
		*count = 0;
		const uintptr_t offset = reinterpret_cast<uintptr_t>(address);
		if (offset == terminate_address)
		{
			++writes;
			if (size != sizeof(target) && fail_redirect)
			{
				// Model an unsuccessful publication without executing callback bytes.
				SetLastError(ERROR_PARTIAL_COPY);
				return FALSE;
			}
			if (size > sizeof(target))
				return FALSE;
			memcpy(target, input, size);
		}
		else
		{
			if (offset < allocation || !range_fits(memory.size(), offset - allocation, size))
				return FALSE;
			memcpy(memory.data() + offset - allocation, input, size);
		}
		*count = size;
		return TRUE;
	}
	BOOL WINAPI protect(HANDLE, LPVOID address, SIZE_T, DWORD value, PDWORD previous)
	{
		*previous = protection;
		if (reinterpret_cast<uintptr_t>(address) == terminate_address)
			protection = value;
		return TRUE;
	}
	LPVOID WINAPI allocate(HANDLE, LPVOID, SIZE_T size, DWORD, DWORD)
	{
		return size == memory.size() ? reinterpret_cast<LPVOID>(allocation) : NULL;
	}
	BOOL WINAPI free(HANDLE, LPVOID address, SIZE_T, DWORD)
	{
		require(reinterpret_cast<uintptr_t>(address) == allocation, "unexpected allocation freed");
		++frees;
		return TRUE;
	}
	HANDLE WINAPI create_event(LPSECURITY_ATTRIBUTES, BOOL manual, BOOL signaled, LPCWSTR)
	{
		require(manual && !signaled, "release event is not initially-unsignaled manual-reset");
		return fail_event ? NULL : reinterpret_cast<HANDLE>(0x40000);
	}
	BOOL WINAPI duplicate(HANDLE, HANDLE source, HANDLE, LPHANDLE target_handle, DWORD access, BOOL, DWORD flags)
	{
		if (flags & DUPLICATE_CLOSE_SOURCE)
		{
			++remote_closes;
			*target_handle = reinterpret_cast<HANDLE>(0x60000);
		}
		else
		{
			require(source == reinterpret_cast<HANDLE>(0x40000) && access == SYNCHRONIZE,
				"target received unexpected event rights");
			*target_handle = reinterpret_cast<HANDLE>(0x50000);
		}
		return TRUE;
	}
	BOOL WINAPI close(HANDLE)
	{
		++local_closes;
		return TRUE;
	}
	BOOL WINAPI signal(HANDLE event)
	{
		require(event == reinterpret_cast<HANDLE>(0x40000), "signaled target-supplied handle");
		require(memcmp(target, original, sizeof(target)) == 0, "released before restoring terminate entry");
		require(protection == PAGE_EXECUTE_READ, "released before restoring protection");
		++signals;
		return TRUE;
	}
	BOOL WINAPI flush(HANDLE, LPCVOID, SIZE_T) { return TRUE; }
}

#define terminate_monitor_hook tested_terminate_monitor_hook
#define VirtualQueryEx hook_mock::query
#define ReadProcessMemory hook_mock::read
#define WriteProcessMemory hook_mock::write
#define VirtualProtectEx hook_mock::protect
#define VirtualAllocEx hook_mock::allocate
#define VirtualFreeEx hook_mock::free
#define CreateEventW hook_mock::create_event
#define DuplicateHandle hook_mock::duplicate
#define CloseHandle hook_mock::close
#define SetEvent hook_mock::signal
#define FlushInstructionCache hook_mock::flush
#include "..\pd\terminate_monitor_hook.cpp"
#undef terminate_monitor_hook
#undef VirtualQueryEx
#undef ReadProcessMemory
#undef WriteProcessMemory
#undef VirtualProtectEx
#undef VirtualAllocEx
#undef VirtualFreeEx
#undef CreateEventW
#undef DuplicateHandle
#undef CloseHandle
#undef SetEvent
#undef FlushInstructionCache

static void add_mock_exports(export_list& exports, bool win64)
{
	char ntdll[] = "ntdll.dll", terminate[] = "NtTerminateProcess";
	char kernelbase[] = "kernelbase.dll", wait[] = "WaitForSingleObject";
	export_entry termination(ntdll, terminate, 1, 0, hook_mock::terminate_address, win64);
	export_entry waiting(kernelbase, wait, 2, 0, hook_mock::wait_address, win64);
	exports.add_export(termination.address, &termination);
	exports.add_export(waiting.address, &waiting);
}

static void hook_lifecycle(bool win64)
{
	hook_mock::reset();
	test_options options;
	export_list exports;
	add_mock_exports(exports, win64);
	tested_terminate_monitor_hook hook(GetCurrentProcess(), GetCurrentProcessId(), win64, &options);
	require(hook.hook_terminate(&exports), "hook installation failed");
	require(hook_mock::protection == PAGE_EXECUTE_READ, "installed hook left code writable");
	require(hook_mock::target[0] == (win64 ? 0xff : 0xe9), "incorrect redirect encoding");
	require(hook.unhock_terminate(), "hook removal failed");
	require(hook_mock::signals == 1, "hook did not release a not-yet-waiting callback");
	require(hook_mock::frees == 0 && hook_mock::remote_closes == 0, "in-flight callback resources were freed");
	require(hook.unhock_terminate() && hook_mock::signals == 1, "hook removal is not idempotent");
}

static void failed_hook_read()
{
	hook_mock::reset();
	hook_mock::fail_read = true;
	test_options options;
	export_list exports;
	add_mock_exports(exports, true);
	{
		tested_terminate_monitor_hook hook(GetCurrentProcess(), GetCurrentProcessId(), true, &options);
		require(!hook.hook_terminate(&exports), "hook accepted a failed original-code read");
	}
	require(hook_mock::writes == 0, "failed original read caused a restore write");
}

static void failed_hook_publication()
{
	hook_mock::reset();
	hook_mock::fail_redirect = true;
	test_options options;
	export_list exports;
	add_mock_exports(exports, true);
	tested_terminate_monitor_hook hook(GetCurrentProcess(), GetCurrentProcessId(), true, &options);
	require(!hook.hook_terminate(&exports), "hook accepted unsuccessful publication");
	require(memcmp(hook_mock::target, hook_mock::original, sizeof(hook_mock::target)) == 0, "failed hook did not roll back");
	require(hook_mock::signals == 1 && hook_mock::protection == PAGE_EXECUTE_READ, "failed publication not released safely");
}

void append_hook_tests(std::vector<test_case>& tests)
{
	tests.emplace_back("hook32-lifecycle", [] { hook_lifecycle(false); });
	tests.emplace_back("hook64-lifecycle", [] { hook_lifecycle(true); });
	tests.emplace_back("hook-failed-read", failed_hook_read);
	tests.emplace_back("hook-failed-publication", failed_hook_publication);
}
