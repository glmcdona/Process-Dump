#pragma once

#include "work_pool.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <windows.h>
#include <tlhelp32.h>

inline std::vector<DWORD> system_processes()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Unable to enumerate processes: Windows error " + std::to_string(GetLastError()));
	std::unique_ptr<void, decltype(&CloseHandle)> close(snapshot, CloseHandle);
	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof(entry);
	std::vector<DWORD> result;
	BOOL more = Process32First(snapshot, &entry);
	while (more)
	{
		result.push_back(entry.th32ProcessID);
		more = Process32Next(snapshot, &entry);
	}
	if (GetLastError() != ERROR_NO_MORE_FILES)
		throw std::runtime_error("Incomplete process enumeration: Windows error " + std::to_string(GetLastError()));
	return result;
}

template<typename Enumerate, typename Action>
void system_work(work_pool& pool, bool rescan, Enumerate enumerate, Action action)
{
	std::unordered_set<DWORD> seen;
	// Drain all producer and module work before the one final discovery pass.
	for (int pass = 0; pass < (rescan ? 2 : 1); ++pass)
	{
		for (DWORD pid : enumerate())
			if (seen.insert(pid).second)
				pool.submit([=, &pool] { action(pid, pool); });
		pool.wait();
	}
}
