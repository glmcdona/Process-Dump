#pragma once

#include <windows.h>
#include <stdint.h>

static const SIZE_T MAX_PE_IMAGE_SIZE = 256u * 1024u * 1024u;

inline bool range_fits(SIZE_T length, SIZE_T offset, SIZE_T count)
{
	return offset <= length && count <= length - offset;
}

static bool test_read( unsigned char* buffer, SIZE_T length, unsigned char* read_ptr, SIZE_T read_length )
{
	const uintptr_t start = reinterpret_cast<uintptr_t>(buffer);
	const uintptr_t address = reinterpret_cast<uintptr_t>(read_ptr);
	return buffer != NULL && read_ptr != NULL && address >= start &&
		range_fits(length, address - start, read_length);
};


static bool write_memory(HANDLE ph, unsigned __int64 address, unsigned __int64 value)
{
	SIZE_T num_written = 0;
	return WriteProcessMemory(ph, (LPVOID) address, &value, sizeof(value), &num_written) && num_written == sizeof(value);
};

static bool write_memory(HANDLE ph, unsigned __int64 address, unsigned __int32 value)
{
	SIZE_T num_written = 0;
	return WriteProcessMemory(ph, (LPVOID)address, &value, sizeof(value), &num_written) && num_written == sizeof(value);
};

static bool read_memory(HANDLE ph, unsigned __int64 address, unsigned __int64* value)
{
	SIZE_T num_read = 0;
	return ReadProcessMemory(ph, (LPVOID)address, value, sizeof(*value), &num_read) && num_read == sizeof(*value);
};

static bool read_memory(HANDLE ph, unsigned __int64 address, unsigned __int32* value)
{
	SIZE_T num_read = 0;
	return ReadProcessMemory(ph, (LPVOID)address, value, sizeof(*value), &num_read) && num_read == sizeof(*value);
};

static bool read_memory(HANDLE ph, unsigned __int64 address, void** value)
{
	SIZE_T num_read = 0;
	return ReadProcessMemory(ph, (LPVOID)address, value, sizeof(*value), &num_read) && num_read == sizeof(*value);
};

static bool read_memory(HANDLE ph, unsigned __int64 address, DWORD* value)
{
	SIZE_T num_read = 0;
	return ReadProcessMemory(ph, (LPVOID)address, value, sizeof(*value), &num_read) && num_read == sizeof(*value);
};