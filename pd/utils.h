#pragma once

static bool test_read( unsigned char* buffer, SIZE_T length, unsigned char* read_ptr, SIZE_T read_length )
{
	return read_ptr >= buffer && read_ptr + read_length <= buffer + length;
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