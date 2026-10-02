#include "test_support.h"

class virtual_buffer
{
public:
	unsigned char* bytes;
	SIZE_T size;
	explicit virtual_buffer(SIZE_T length) : size(length)
	{
		bytes = static_cast<unsigned char*>(VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		require(bytes != NULL, "test VirtualAlloc failed");
	}
	~virtual_buffer() { VirtualFree(bytes, 0, MEM_RELEASE); }
	virtual_buffer(const virtual_buffer&) = delete;
	virtual_buffer& operator=(const virtual_buffer&) = delete;
};

static void interior_region_read()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	const SIZE_T page = info.dwPageSize;
	virtual_buffer source(page);
	memset(source.bytes, 0x5a, source.size);
	std::vector<unsigned char> destination(32, 0);
	process_stream stream(GetCurrentProcess(), source.bytes + 17);
	SIZE_T read = 0;
	require(stream.read(0, destination.size(), destination.data(), &read) && read == destination.size(), "small region read failed");
	for (SIZE_T i = 0; i < destination.size(); ++i)
		require(destination[i] == 0x5a, "interior region contents changed");
	require(stream.block_size(0) == page - 17, "block_size includes bytes before base");
	require(stream.estimate_section_size(0) == page - 17, "section estimate includes bytes before base");
}

static void sparse_region_read()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	const SIZE_T page = info.dwPageSize;
	virtual_buffer source(3 * page);
	memset(source.bytes, 0x5a, source.size);
	DWORD previous;
	require(VirtualProtect(source.bytes + page, page, PAGE_NOACCESS, &previous) != 0, "test noaccess setup failed");
	process_stream stream(GetCurrentProcess(), source.bytes);
	std::vector<unsigned char> destination(source.size, 0xcc);
	SIZE_T read = 0;
	require(!stream.read(0, destination.size(), destination.data(), &read), "sparse read falsely reported complete");
	require(read == 2 * page, "sparse read count changed");
	for (SIZE_T i = 0; i < destination.size(); ++i)
		require(destination[i] == (i >= page && i < 2 * page ? 0xcc : 0x5a), "sparse read overwrote skipped bytes");
}

static void file_stream_bounds()
{
	temporary_file file;
	file.write(std::vector<unsigned char>{1, 2, 3});
	file_stream stream(file.path);
	require(stream.block_size(4) == 0 && stream.block_size(-1) == 0, "file block size underflow");
	unsigned char value = 0xcc;
	SIZE_T read = 99;
	require(!stream.read(-1, 1, &value, &read) && read == 0 && value == 0xcc, "negative file offset accepted");
	char name = 'x';
	require(stream.get_short_name(&name, 0) == 0 && name == 'x', "zero-capacity name write");
	require(stream.get_long_name(&name, 0) == 0 && name == 'x', "zero-capacity path write");
}

static void borrowed_process_handle()
{
	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	require(handle != NULL, "test OpenProcess failed");
	{
		unsigned char value = 42;
		process_stream stream(handle, &value);
		unsigned char out = 0;
		SIZE_T read = 0;
		require(stream.read(0, 1, &out, &read) && out == 42, "borrowed process read failed");
	}
	DWORD exit_code = 0;
	const BOOL valid = GetExitCodeProcess(handle, &exit_code);
	CloseHandle(handle);
	require(valid != 0, "stream closed borrowed handle");
}

void append_stream_tests(std::vector<test_case>& tests)
{
	tests.emplace_back("stream-interior-region", interior_region_read);
	tests.emplace_back("stream-sparse-regions", sparse_region_read);
	tests.emplace_back("stream-file-bounds", file_stream_bounds);
	tests.emplace_back("stream-borrowed-handle", borrowed_process_handle);
}
