#pragma once

#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

class dump_directory_locks
{
	std::vector<HANDLE> handles;
	bool accept(HANDLE handle)
	{
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
public:
	HANDLE root() const { return handles.empty() ? INVALID_HANDLE_VALUE : handles.front(); }

	~dump_directory_locks()
	{
		for (HANDLE handle : handles)
			CloseHandle(handle);
	}

	bool hold(const std::string& path)
	{
		return accept(CreateFileA(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
	}
	bool hold_relative(const std::string& path);
};

namespace dump_output
{
	inline HANDLE open_no_reparse(HANDLE root, const char* relative_path, bool directory,
		decltype(&NtCreateFile) create_file = NtCreateFile)
	{
		if (root == NULL || root == INVALID_HANDLE_VALUE || relative_path == NULL ||
			relative_path[0] == 0 || relative_path[0] == '\\' || relative_path[0] == '/' ||
			strchr(relative_path, ':') != NULL)
		{
			SetLastError(ERROR_INVALID_NAME);
			return INVALID_HANDLE_VALUE;
		}
		WCHAR name_buffer[MAX_PATH];
		const int length = MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, MB_ERR_INVALID_CHARS,
			relative_path, -1, name_buffer, MAX_PATH);
		if (length == 0)
			return INVALID_HANDLE_VALUE;
		UNICODE_STRING name = {};
		name.Buffer = name_buffer;
		name.Length = static_cast<USHORT>((length - 1) * sizeof(WCHAR));
		name.MaximumLength = static_cast<USHORT>(length * sizeof(WCHAR));
		OBJECT_ATTRIBUTES attributes = {};
		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = root;
		attributes.ObjectName = &name;
		attributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE;
		IO_STATUS_BLOCK io = {};
		HANDLE file = INVALID_HANDLE_VALUE;
		const NTSTATUS status = create_file(&file,
			(directory ? FILE_READ_ATTRIBUTES : GENERIC_WRITE | DELETE) | SYNCHRONIZE, &attributes, &io,
			NULL, FILE_ATTRIBUTE_NORMAL, directory ? FILE_SHARE_READ | FILE_SHARE_WRITE : 0,
			directory ? FILE_OPEN : FILE_CREATE,
			(directory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE) |
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT, NULL, 0);
		if (status < 0)
		{
			SetLastError(RtlNtStatusToDosError(status));
			return INVALID_HANDLE_VALUE;
		}
		return file;
	}

	inline HANDLE create_no_reparse(HANDLE root, const char* relative_path,
		decltype(&NtCreateFile) create_file = NtCreateFile)
	{
		return open_no_reparse(root, relative_path, false, create_file);
	}
}

inline bool dump_directory_locks::hold_relative(const std::string& path)
{
	return accept(dump_output::open_no_reparse(root(), path.c_str(), true));
}

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
	// These handles prevent renames, but cannot prevent in-place reparse-point changes.
	dump_directory_locks locks;
	if (!locks.hold(path.substr(0, root_end)))
	{
		fprintf(stderr, "ERROR: Cannot secure dump root '%s' (Windows error %lu).\n", filename, GetLastError());
		return false;
	}
	for (SIZE_T end = path.find('\\', root_end); end != std::string::npos; end = path.find('\\', end + 1))
	{
		if (!locks.hold_relative(path.substr(root_end, end - root_end)))
		{
			fprintf(stderr, "ERROR: Dump directory is unavailable or contains a reparse point: '%s' (Windows error %lu).\n",
				filename, GetLastError());
			return false;
		}
	}
	// Reject reparses during the actual create, not only during the earlier directory checks.
	HANDLE file = dump_output::create_no_reparse(locks.root(), path.c_str() + root_end);
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
