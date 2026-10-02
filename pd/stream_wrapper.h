#pragma once

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <algorithm>
#include "windows.h"
#include "simple.h"
#include <tlhelp32.h>
#include "module_list.h"

class stream_wrapper
{
public:
	bool file_alignment;
	virtual SIZE_T block_size(long offset) = 0;
	virtual bool read(long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read) = 0;
	virtual SIZE_T get_short_name(char* out_name, SIZE_T out_name_size) = 0;
	virtual SIZE_T get_long_name(char* out_name, SIZE_T out_name_size) = 0;
	virtual SIZE_T get_location(char* out_name, SIZE_T out_name_size) = 0;
	virtual __int64 get_address() = 0;
	virtual __int64 estimate_section_size(long offset) = 0;
	virtual DWORD get_region_characteristics(long offset) = 0;
	virtual ~stream_wrapper() {}
	virtual void update_base(__int64 rva) = 0;
};

inline SIZE_T copy_stream_name(const char* name, char* output, SIZE_T capacity)
{
	if (output == NULL || capacity == 0)
		return 0;
	output[0] = 0;
	if (name == NULL)
		return 0;
	const SIZE_T length = (std::min<SIZE_T>)(strlen(name), capacity - 1);
	memcpy(output, name, length);
	output[length] = 0;
	return length;
}

class file_stream : public stream_wrapper
{
	char* _filename;
	FILE* fh;
public:
	file_stream(char* filename) : fh(NULL)
	{
		_filename = new char[strlen(filename) + 1];
		strcpy(_filename, filename);
		fh = fopen(filename, "rb");
		file_alignment = true;
		if (fh == NULL)
			fprintf(stderr, "ERROR: Failed to open input file '%s'.\n", filename);
	}

	virtual void update_base(__int64) {}
	virtual __int64 get_address() { return 0; }
	virtual __int64 estimate_section_size(long) { return 0; }
	virtual SIZE_T get_location(char* output, SIZE_T capacity) { return get_long_name(output, capacity); }
	virtual SIZE_T get_long_name(char* output, SIZE_T capacity)
	{
		return copy_stream_name(_filename, output, capacity);
	}
	virtual SIZE_T get_short_name(char* output, SIZE_T capacity)
	{
		char fname[_MAX_FNAME] = {}, ext[_MAX_EXT] = {};
		char name[_MAX_FNAME + _MAX_EXT + 1] = {};
		if (_splitpath_s(_filename, NULL, 0, NULL, 0, fname, sizeof(fname), ext, sizeof(ext)) != 0)
		{
			fprintf(stderr, "ERROR: Input filename is too long.\n");
			return copy_stream_name(NULL, output, capacity);
		}
		// Preserve the historical display name, including the extra dot before the extension.
		sprintf_s(name, sizeof(name), "%s.%s", fname, ext);
		return copy_stream_name(name, output, capacity);
	}
	virtual SIZE_T block_size(long offset)
	{
		if (fh == NULL || offset < 0)
			return 0;
		if (_fseeki64(fh, 0, SEEK_END) != 0)
		{
			fprintf(stderr, "ERROR: Failed to seek input file.\n");
			return 0;
		}
		const __int64 end = _ftelli64(fh);
		if (end < offset)
			return 0;
		const unsigned __int64 remaining = static_cast<unsigned __int64>(end - offset);
		return static_cast<SIZE_T>((std::min)(remaining, static_cast<unsigned __int64>(SIZE_MAX)));
	}
	virtual DWORD get_region_characteristics(long)
	{
		return IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
	}
	virtual bool read(long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read)
	{
		if (out_read == NULL)
			return false;
		*out_read = 0;
		if (fh == NULL || offset < 0 || (output == NULL && size != 0))
			return false;
		if (_fseeki64(fh, offset, SEEK_SET) != 0)
			return false;
		*out_read = fread(output, 1, size, fh);
		return *out_read == size;
	}
	~file_stream()
	{
		if (fh != NULL)
			fclose(fh);
		delete[] _filename;
	}
	file_stream(const file_stream&) = delete;
	file_stream& operator=(const file_stream&) = delete;
};

class process_stream : public stream_wrapper
{
	bool opened = false;
	bool owns_handle = false;
	HANDLE ph = NULL;
	char* _long_name = NULL;
	char* _short_name = NULL;

	void init(HANDLE handle, void* address, module_list* modules)
	{
		file_alignment = false;
		ph = handle;
		base = address;
		opened = handle != NULL && address != NULL;
		if (!opened || modules == NULL)
			return;
		const auto item = modules->_modules.find(reinterpret_cast<uintptr_t>(base));
		if (item != modules->_modules.end())
		{
			_long_name = new char[260];
			_short_name = new char[256];
			copy_stream_name(item->second->full_name, _long_name, 260);
			copy_stream_name(item->second->short_name, _short_name, 256);
		}
	}

	bool address_at(long offset, uintptr_t& address) const
	{
		const uintptr_t start = reinterpret_cast<uintptr_t>(base);
		if (!opened || offset < 0 || static_cast<uintptr_t>(offset) > UINTPTR_MAX - start)
			return false;
		address = start + static_cast<uintptr_t>(offset);
		return true;
	}

	bool query(long offset, MEMORY_BASIC_INFORMATION& region, uintptr_t& address)
	{
		return address_at(offset, address) &&
			VirtualQueryEx(ph, reinterpret_cast<LPCVOID>(address), &region, sizeof(region)) == sizeof(region);
	}

public:
	void* base = NULL;

	process_stream(HANDLE handle, void* address) { init(handle, address, NULL); }
	process_stream(HANDLE handle, void* address, module_list* modules) { init(handle, address, modules); }

	process_stream(DWORD pid, void* address, module_list* modules)
	{
		owns_handle = true;
		init(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid), address, modules);
		if (ph == NULL)
			PrintLastError(L"Opening process stream");
	}

	process_stream(DWORD pid, module_list* modules)
	{
		file_alignment = false;
		owns_handle = true;
		ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (ph == NULL)
		{
			PrintLastError(L"Opening process stream");
			return;
		}
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			PrintLastError(L"Snapshotting process modules");
			return;
		}
		MODULEENTRY32 entry = {};
		entry.dwSize = sizeof(entry);
		if (Module32First(snapshot, &entry))
			init(ph, entry.modBaseAddr, modules);
		else
			PrintLastError(L"Reading first process module");
		CloseHandle(snapshot);
	}

	virtual SIZE_T get_location(char* output, SIZE_T capacity)
	{
		char name[19];
		sprintf_s(name, sizeof(name), "0x%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(base)));
		return copy_stream_name(name, output, capacity);
	}
	virtual SIZE_T get_short_name(char* output, SIZE_T capacity)
	{
		return copy_stream_name(_short_name, output, capacity);
	}
	virtual SIZE_T get_long_name(char* output, SIZE_T capacity)
	{
		return copy_stream_name(_long_name, output, capacity);
	}
	virtual SIZE_T block_size(long offset)
	{
		MEMORY_BASIC_INFORMATION region = {};
		uintptr_t address = 0;
		if (!query(offset, region, address))
			return 0;
		const uintptr_t start = reinterpret_cast<uintptr_t>(region.BaseAddress);
		if (address < start || address - start >= region.RegionSize)
			return 0;
		return region.RegionSize - (address - start);
	}
	virtual DWORD get_region_characteristics(long offset)
	{
		MEMORY_BASIC_INFORMATION region = {};
		uintptr_t address = 0;
		DWORD result = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		if (query(offset, region, address) && region.State == MEM_COMMIT &&
			!(region.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
			(region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
			result |= IMAGE_SCN_MEM_EXECUTE;
		return result;
	}
	virtual __int64 estimate_section_size(long offset)
	{
		MEMORY_BASIC_INFORMATION region = {};
		uintptr_t address = 0;
		if (!query(offset, region, address) || region.State != MEM_COMMIT ||
			(region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
			return 0;
		const uintptr_t start = reinterpret_cast<uintptr_t>(region.BaseAddress);
		if (address < start || address - start >= region.RegionSize)
			return 0;
		return region.RegionSize - (address - start);
	}
	virtual void update_base(__int64 rva)
	{
		const uintptr_t address = reinterpret_cast<uintptr_t>(base);
		const unsigned __int64 magnitude = rva < 0 ? static_cast<unsigned __int64>(-(rva + 1)) + 1 : rva;
		if ((rva < 0 && magnitude > address) || (rva >= 0 && magnitude > UINTPTR_MAX - address))
		{
			fprintf(stderr, "ERROR: Process stream base adjustment is out of range.\n");
			opened = false;
			return;
		}
		base = reinterpret_cast<void*>(rva < 0 ? address - static_cast<uintptr_t>(magnitude) : address + static_cast<uintptr_t>(magnitude));
	}
	virtual __int64 get_address() { return reinterpret_cast<uintptr_t>(base); }

	virtual bool read(long offset, SIZE_T size, unsigned char* output, SIZE_T* out_read)
	{
		if (out_read == NULL)
			return false;
		*out_read = 0;
		uintptr_t address = 0;
		if (!address_at(offset, address) || (output == NULL && size != 0) || size > UINTPTR_MAX - address)
			return false;

		SIZE_T processed = 0;
		bool complete = true;
		while (processed < size)
		{
			MEMORY_BASIC_INFORMATION region = {};
			const uintptr_t current = address + processed;
			if (VirtualQueryEx(ph, reinterpret_cast<LPCVOID>(current), &region, sizeof(region)) != sizeof(region))
				return false;
			const uintptr_t start = reinterpret_cast<uintptr_t>(region.BaseAddress);
			if (current < start || current - start >= region.RegionSize)
				return false;
			const SIZE_T count = (std::min)(size - processed, region.RegionSize - (current - start));
			if (region.State == MEM_COMMIT && !(region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
			{
				SIZE_T read = 0;
				const BOOL success = ReadProcessMemory(ph, reinterpret_cast<LPCVOID>(current), output + processed, count, &read);
				*out_read += read;
				if (!success || read != count)
					complete = false;
			}
			else
			{
				// Preserve unreadable spans so the caller's zero-filled image stays sparse.
				complete = false;
			}
			processed += count;
		}
		return complete && *out_read == size;
	}
	~process_stream()
	{
		if (owns_handle && ph != NULL)
			CloseHandle(ph);
		delete[] _long_name;
		delete[] _short_name;
	}
	process_stream(const process_stream&) = delete;
	process_stream& operator=(const process_stream&) = delete;
};
