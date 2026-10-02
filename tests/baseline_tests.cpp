#define NMD_ASSEMBLY_IMPLEMENTATION
#include "test_support.h"
#include "hash.h"
#include "work_queue.h"
#include <functional>
#include <crtdbg.h>

bool global_flag_verbose = false;

static void crc_vectors()
{
	char text[] = "123456789";
	require(crc32buf(text, 9) == 0xcbf43926, "CRC standard vector changed");
	require(crc32buf(text, 0) == 0, "CRC empty vector changed");
	DWORD crc = 0xffffffff;
	for (char ch : std::string(text))
		crc = updateCRC32(static_cast<unsigned char>(ch), crc);
	require(~crc == 0xcbf43926, "CRC incremental vector changed");
}

static void file_stream_roundtrip()
{
	temporary_file file;
	file.write(std::vector<unsigned char>{1, 2, 3, 4, 5});
	file_stream stream(file.path);
	require(stream.block_size(0) == 5 && stream.block_size(2) == 3, "file length changed");
	unsigned char bytes[3] = {};
	SIZE_T read = 0;
	require(stream.read(1, sizeof(bytes), bytes, &read), "file read failed");
	require(read == 3 && bytes[0] == 2 && bytes[2] == 4, "file contents changed");
}

static void process_stream_roundtrip()
{
	unsigned char source[16] = {1, 2, 3, 4};
	process_stream stream(GetCurrentProcess(), source);
	unsigned char bytes[4] = {};
	SIZE_T read = 0;
	require(stream.read(0, sizeof(bytes), bytes, &read), "self-process read failed");
	require(read == sizeof(bytes) && memcmp(source, bytes, sizeof(bytes)) == 0, "process contents changed");
}

static void pe_roundtrip(bool win64, bool imports)
{
	temporary_file source, output;
	source.write(pe_fixture(win64));
	test_options options;
	options.ImportRec = imports;
	pe_header header(source.path, &options);
	require(header.process_pe_header(), "valid PE header rejected");
	require(header.is_64() == win64 && header.is_exe() && !header.is_dll(), "PE type changed");
	require(header.process_sections(), "valid sections rejected");
	require(header.get_virtual_size() == 0x2000, "virtual size changed");
	header.process_import_directory();
	header.process_export_directory();
	export_list exports;
	require(header.process_disk_image(&exports, NULL), "disk reconstruction failed");
	require(DeleteFileA(output.path) != 0, "could not reserve unused output filename");
	require(header.write_image(output.path), "dump write failed");
	std::vector<unsigned char> bytes = output.read();
	require(bytes.size() >= 0x2000, "dump truncated");
	require(bytes[0] == 'M' && bytes[1] == 'Z' && bytes[0x1000] == 0x90, "dump data changed");
	printf("FINGERPRINT PE%s imports=%d size=%zu crc=%08lx hash=%016llx\n",
		win64 ? "64" : "32", imports, bytes.size(),
		crc32buf(reinterpret_cast<char*>(bytes.data()), bytes.size()), header.get_hash());
	const DWORD expected_crc = imports ? (win64 ? 0x64a456d2 : 0xd9e601a9) : (win64 ? 0x0f6a2ed3 : 0xfd941dbd);
	require(bytes.size() == (imports ? 12288 : 8192), "baseline dump size changed");
	require(crc32buf(reinterpret_cast<char*>(bytes.data()), bytes.size()) == expected_crc, "baseline dump bytes changed");
	require(header.get_hash() == (imports ? 0x391c57a138fafe6cULL : 0xcd8757e79d476f71ULL), "baseline PE hash changed");
}

static void import_table_roundtrip(bool win64)
{
	pe_imports imports(NULL, 0, NULL, win64);
	char library[] = "kernel32.dll", function[] = "ExitProcess";
	imports.add_fixup(library, function, 0x1000, win64);
	__int64 descriptors = 0, extra = 0;
	imports.get_table_size(descriptors, extra);
	std::vector<unsigned char> bytes(static_cast<size_t>(descriptors + extra), 0);
	require(imports.build_table(bytes.data(), bytes.size(), 0x3000, 0, descriptors), "import table failed");
	IMAGE_IMPORT_DESCRIPTOR* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(bytes.data());
	require(strcmp(reinterpret_cast<char*>(bytes.data() + descriptor->Name - 0x3000), library) == 0, "import library changed");
	require(bytes.size() == 83 && crc32buf(reinterpret_cast<char*>(bytes.data()), bytes.size()) == 0x1e081ba4, "baseline import bytes changed");
	printf("FINGERPRINT IMPORT%s size=%zu crc=%08lx\n", win64 ? "64" : "32",
		bytes.size(), crc32buf(reinterpret_cast<char*>(bytes.data()), bytes.size()));
}

static void export_lookup()
{
	export_list exports;
	char library[] = "kernel32.dll", function[] = "ExitProcess";
	export_entry entry(library, function, 1, 0x5678, 0x12345678, false);
	exports.add_export(0x12345678, &entry);
	require(exports.contains(static_cast<unsigned __int32>(0x12345678)), "export lookup failed");
	require(exports.find_export(library, function, false) == 0x12345678, "named export lookup failed");
	require(!exports.contains(static_cast<unsigned __int32>(0x12345679)), "missing export found");
}

static void database_roundtrip()
{
	temporary_file clean, ep, short_ep;
	{
		pe_hash_database db(clean.path, ep.path, short_ep.path);
		require(db.add_hashes({0, 0x1234, 0x1234, 0x5678}), "hash insertion failed");
		require(db.add_hashes_eps({0, 0xabc}, {0, 0xdef}), "entrypoint insertion failed");
		require(db.count() == 2 && db.contains(0x1234) && !db.contains(0), "database semantics changed");
		require(db.save(), "database save failed");
	}
	pe_hash_database db(clean.path, ep.path, short_ep.path);
	require(db.count() == 2 && db.contains(0x5678), "clean reload changed");
	require(db.count_eps() == 1 && db.contains_ep(0xabc), "entrypoint reload changed");
	require(db.count_epshorts() == 1 && db.contains_epshort(0xdef), "short entrypoint reload changed");
}

static void queue_fifo()
{
	Queue<int> queue;
	int value = 0;
	require(!queue.pop(value), "empty queue pop succeeded");
	queue.push(10);
	queue.push(20);
	require(queue.count() == 2 && queue.pop() == 10 && queue.pop(value) && value == 20, "queue FIFO changed");
	require(queue.empty(), "queue not empty");
}

int main(int argc, char** argv)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
	setvbuf(stdout, NULL, _IONBF, 0);
	std::vector<test_case> tests = {
		{"crc-vectors", crc_vectors}, {"file-stream", file_stream_roundtrip},
		{"process-stream", process_stream_roundtrip},
		{"pe32-roundtrip", [] { pe_roundtrip(false, false); }},
		{"pe64-roundtrip", [] { pe_roundtrip(true, false); }},
		{"pe32-import-reconstruction", [] { pe_roundtrip(false, true); }},
		{"pe64-import-reconstruction", [] { pe_roundtrip(true, true); }},
		{"imports32", [] { import_table_roundtrip(false); }},
		{"imports64", [] { import_table_roundtrip(true); }},
		{"export-lookup", export_lookup}, {"database-roundtrip", database_roundtrip},
		{"queue-fifo", queue_fifo}
	};
	if (argc > 1 && strcmp(argv[1], "--security") == 0)
		tests.clear();
	if (argc == 1 || strcmp(argv[1], "--baseline") != 0)
	{
		append_stream_tests(tests);
		append_pe_safety_tests(tests);
		append_name_safety_tests(tests);
		append_hook_tests(tests);
		append_output_tests(tests);
		append_reconstruction_tests(tests);
		append_pipeline_tests(tests);
	}
	int failures = 0, count = 0;
	for (const auto& test : tests)
	{
		if (argc > 1 && strcmp(argv[1], test.first) != 0 && strcmp(argv[1], "--baseline") != 0 && strcmp(argv[1], "--security") != 0)
			continue;
		++count;
		try { test.second(); printf("PASS %s\n", test.first); }
		catch (const std::exception& error) { ++failures; fprintf(stderr, "FAIL %s: %s\n", test.first, error.what()); }
	}
	printf("RESULT %d tests, %d failures\n", count, failures);
	return failures || count == 0 ? 1 : 0;
}
