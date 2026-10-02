#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>

class dump_directory_locks
{
	std::vector<HANDLE> handles;
public:
	~dump_directory_locks()
	{
		for (HANDLE handle : handles)
			CloseHandle(handle);
	}

	bool hold(const std::string& path)
	{
		HANDLE handle = CreateFileA(path.c_str(), FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return false;
		BY_HANDLE_FILE_INFORMATION info = {};
		if (!GetFileInformationByHandle(handle, &info) ||
			!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
			(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
		{
			CloseHandle(handle);
			SetLastError(ERROR_ACCESS_DENIED);
			return false;
		}
		handles.push_back(handle);
		return true;
	}
};

inline bool write_new_dump(const char* filename, const unsigned char* data, SIZE_T size)
{
	if (filename == NULL || data == NULL || size == 0)
	{
		fprintf(stderr, "ERROR: No reconstructed image is available to write.\n");
		return false;
	}
	char full_path[MAX_PATH];
	const DWORD length = GetFullPathNameA(filename, MAX_PATH, full_path, NULL);
	if (length == 0 || length >= MAX_PATH)
	{
		fprintf(stderr, "ERROR: Dump path is invalid or exceeds MAX_PATH: '%s'.\n", filename);
		return false;
	}
	const std::string path(full_path);
	SIZE_T root_end;
	if (path.size() >= 3 && path[1] == ':' && path[2] == '\\')
		root_end = 3;
	else if (path.compare(0, 2, "\\\\") == 0 && path.size() > 2 && path[2] != '?' && path[2] != '.')
	{
		const SIZE_T server_end = path.find('\\', 2);
		root_end = server_end == std::string::npos ? std::string::npos : path.find('\\', server_end + 1);
		if (root_end != std::string::npos)
			++root_end;
	}
	else
		root_end = std::string::npos;
	if (root_end == std::string::npos || root_end >= path.size())
	{
		fprintf(stderr, "ERROR: Unsupported dump path: '%s'.\n", filename);
		return false;
	}

	// Keep every parent directory open without delete sharing until creation and writing finish.
	// This prevents a target from replacing a parent with a junction between validation and use.
	dump_directory_locks locks;
	if (!locks.hold(path.substr(0, root_end)))
	{
		fprintf(stderr, "ERROR: Cannot secure dump root '%s' (Windows error %lu).\n", filename, GetLastError());
		return false;
	}
	for (SIZE_T end = path.find('\\', root_end); end != std::string::npos; end = path.find('\\', end + 1))
	{
		if (!locks.hold(path.substr(0, end)))
		{
			fprintf(stderr, "ERROR: Dump directory is unavailable or contains a reparse point: '%s' (Windows error %lu).\n",
				filename, GetLastError());
			return false;
		}
	}
	HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "ERROR: Cannot create dump '%s'; existing files are not overwritten (Windows error %lu).\n",
			filename, GetLastError());
		return false;
	}
	SIZE_T offset = 0;
	bool success = true;
	while (offset < size)
	{
		const DWORD count = static_cast<DWORD>((std::min)(size - offset, static_cast<SIZE_T>(MAXDWORD)));
		DWORD written = 0;
		if (!WriteFile(file, data + offset, count, &written, NULL) || written != count)
		{
			fprintf(stderr, "ERROR: Incomplete dump write to '%s' (Windows error %lu).\n", filename, GetLastError());
			success = false;
			break;
		}
		offset += written;
	}
	if (success && !FlushFileBuffers(file))
	{
		fprintf(stderr, "ERROR: Cannot flush dump '%s' (Windows error %lu).\n", filename, GetLastError());
		success = false;
	}
	if (!success)
	{
		FILE_DISPOSITION_INFO disposition = {};
		disposition.DeleteFile = TRUE;
		if (!SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition)))
			fprintf(stderr, "ERROR: Cannot remove incomplete dump '%s' (Windows error %lu).\n", filename, GetLastError());
	}
	if (!CloseHandle(file))
	{
		fprintf(stderr, "ERROR: Cannot close dump '%s' (Windows error %lu).\n", filename, GetLastError());
		success = false;
	}
	return success;
}
