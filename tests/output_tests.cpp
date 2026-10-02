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

void append_output_tests(std::vector<test_case>& tests)
{
	tests.emplace_back("output-new-file", new_output_roundtrip);
	tests.emplace_back("output-preserve-existing", existing_output_unchanged);
	tests.emplace_back("output-invalid-path", invalid_output_rejected);
}
