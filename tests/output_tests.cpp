#include "test_support.h"
#include "dump_output.h"

static void new_output_roundtrip()
{
	temporary_file output;
	require(DeleteFileA(output.path) != 0, "output fixture removal failed");
	const std::vector<unsigned char> expected = {1, 2, 3, 4, 5};
	require(write_new_dump(output.path, expected.data(), expected.size()), "new dump creation failed");
	require(output.read() == expected, "new dump contents differ");
}

static void existing_output_unchanged()
{
	temporary_file output;
	const std::vector<unsigned char> expected = {1, 2, 3};
	output.write(expected);
	const unsigned char replacement = 9;
	require(!write_new_dump(output.path, &replacement, 1), "existing dump was overwritten");
	require(output.read() == expected, "existing dump changed");
}

static void invalid_output_rejected()
{
	const unsigned char data = 1;
	require(!write_new_dump("\\\\.\\NUL", &data, 1), "device path accepted");
	require(!write_new_dump(NULL, &data, 1), "null output accepted");
}

static unsigned int native_create_calls = 0;
static NTSTATUS native_failure = 0;
static bool native_directory = false;

static NTSTATUS NTAPI checked_native_create(PHANDLE file, ACCESS_MASK access, POBJECT_ATTRIBUTES attributes,
	PIO_STATUS_BLOCK io, PLARGE_INTEGER allocation, ULONG file_attributes, ULONG sharing,
	ULONG disposition, ULONG options, PVOID ea, ULONG ea_size)
{
	++native_create_calls;
	require(file != NULL && io != NULL && allocation == NULL, "native creation output parameters changed");
	require(attributes->RootDirectory == reinterpret_cast<HANDLE>(static_cast<uintptr_t>(42)),
		"creation did not retain the verified root handle");
	require(attributes->Attributes == (OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE),
		"creation does not reject reparses atomically");
	const std::wstring name(attributes->ObjectName->Buffer, attributes->ObjectName->Length / sizeof(WCHAR));
	require(name == L"nested directory\\file.exe" &&
		attributes->ObjectName->MaximumLength == attributes->ObjectName->Length + sizeof(WCHAR),
		"root-relative Unicode name changed");
	require(access == ((native_directory ? FILE_READ_ATTRIBUTES : GENERIC_WRITE | DELETE) | SYNCHRONIZE) &&
		sharing == (native_directory ? FILE_SHARE_READ | FILE_SHARE_WRITE : 0) &&
		disposition == (native_directory ? FILE_OPEN : FILE_CREATE) &&
		options == ((native_directory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE) |
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT) &&
		file_attributes == FILE_ATTRIBUTE_NORMAL && ea == NULL && ea_size == 0,
		"native creation weakened exclusive, synchronous file creation");
	return native_failure;
}

static void native_output_fail_closed()
{
	const HANDLE root = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(42));
	// Exercise API failures, not an attacker-controlled filesystem or a race.
	for (bool directory : {false, true})
	{
		native_directory = directory;
		for (NTSTATUS status : {static_cast<NTSTATUS>(0xc000050bL), static_cast<NTSTATUS>(0xc0000022L),
			static_cast<NTSTATUS>(0xc000000dL)})
		{
			native_failure = status;
			native_create_calls = 0;
			const HANDLE result = directory ?
				dump_output::open_no_reparse(root, "nested directory\\file.exe", true, checked_native_create) :
				dump_output::create_no_reparse(root, "nested directory\\file.exe", checked_native_create);
			require(result == INVALID_HANDLE_VALUE, "native create failure was ignored");
			require(native_create_calls == 1 && GetLastError() == RtlNtStatusToDosError(status),
				"native failure was retried or not propagated");
		}
	}
}

static void native_output_requires_relative_path()
{
	native_create_calls = 0;
	const HANDLE root = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(42));
	for (const char* path : {"", "\\absolute", "/absolute", "C:\\absolute", "file:stream"})
		require(dump_output::create_no_reparse(root, path, checked_native_create) == INVALID_HANDLE_VALUE,
			"non-relative native output path accepted");
	require(dump_output::create_no_reparse(NULL, "file", checked_native_create) == INVALID_HANDLE_VALUE &&
		dump_output::create_no_reparse(INVALID_HANDLE_VALUE, "file", checked_native_create) == INVALID_HANDLE_VALUE &&
		dump_output::create_no_reparse(root, NULL, checked_native_create) == INVALID_HANDLE_VALUE,
		"invalid native creation arguments accepted");
	require(native_create_calls == 0, "invalid native name reached the filesystem");
}

static void nested_output_roundtrip()
{
	struct directory_fixture
	{
		temporary_file root;
		std::string nested, output;
		directory_fixture() : nested(std::string(root.path) + "\\nested directory"), output(nested + "\\file.bin")
		{
			require(DeleteFileA(root.path) && CreateDirectoryA(root.path, NULL), "temporary directory creation failed");
		}
		~directory_fixture()
		{
			DeleteFileA(output.c_str());
			RemoveDirectoryA(nested.c_str());
			RemoveDirectoryA(root.path);
		}
	} directory;
	require(CreateDirectoryA(directory.nested.c_str(), NULL) != 0, "nested directory creation failed");
	const unsigned char data[] = {1, 2, 3};
	require(write_new_dump(directory.output.c_str(), data, sizeof(data)), "nested dump creation failed");
	file_stream input(const_cast<char*>(directory.output.c_str()));
	unsigned char read[sizeof(data)] = {};
	SIZE_T count = 0;
	require(input.read(0, sizeof(read), read, &count) && count == sizeof(data) &&
		memcmp(data, read, sizeof(data)) == 0, "nested output contents changed");
}

void append_output_tests(std::vector<test_case>& tests)
{
	tests.emplace_back("output-new-file", new_output_roundtrip);
	tests.emplace_back("output-preserve-existing", existing_output_unchanged);
	tests.emplace_back("output-invalid-path", invalid_output_rejected);
	tests.emplace_back("output-native-fail-closed", native_output_fail_closed);
	tests.emplace_back("output-native-relative-path", native_output_requires_relative_path);
	tests.emplace_back("output-nested-file", nested_output_roundtrip);
}
