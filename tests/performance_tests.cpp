#include "test_support.h"
#include "pe_hash_database.h"
#include <atomic>
#include <thread>

namespace
{
	std::vector<unsigned char> serialize(pe_imports& imports)
	{
		__int64 descriptors = 0, extra = 0;
		imports.get_table_size(descriptors, extra);
		require(imports.valid() && descriptors >= 0 && extra >= 0, "import records invalid");
		std::vector<unsigned char> bytes(static_cast<SIZE_T>(descriptors + extra), 0);
		require(imports.build_table(bytes.data(), bytes.size(), 0x5000, 0, descriptors), "import serialization failed");
		return bytes;
	}

	void import_storage_equivalence(bool win64)
	{
		export_list exports;
		pe_imports owned(NULL, 0, NULL, win64), borrowed(NULL, 0, NULL, win64);
		IMAGE_IMPORT_DESCRIPTOR original = {};
		original.FirstThunk = 0x1400;
		original.OriginalFirstThunk = 0x1800;
		original.Name = 0x1200;
		owned.add_descriptor(&original);
		borrowed.add_descriptor(&original);
		for (unsigned int i = 0; i < 257; ++i)
		{
			char library[] = "example.dll", name[] = "ExampleFunction";
			const unsigned __int64 address = win64 ? 0x7fff12340000ULL + i : 0x12340000ULL + i;
			export_entry entry(library, i % 2 ? name : NULL, static_cast<WORD>(i + 1), 0x1000, address, win64);
			exports.add_export(address, &entry);
			const auto* found = exports.lookup(address);
			require(found != NULL, "borrowed export lookup failed");
			borrowed.add_fixup(*found, 0x2000 + i * 8, win64);
			if (i % 2)
				owned.add_fixup(library, name, 0x2000 + i * 8, win64);
			else
				owned.add_fixup(library, i + 1, 0x2000 + i * 8, win64);
			memset(library, 'x', sizeof(library) - 1);
			memset(name, 'x', sizeof(name) - 1);
		}
		original.FirstThunk = 0;
		require(serialize(owned) == serialize(borrowed), "moved/borrowed import storage changed serialized bytes");
	}

	void export_transfer()
	{
		export_list destination, source;
		export_entry existing("first.dll", "First", 1, 0x1000, 0x12340000, false);
		export_entry duplicate("second.dll", "Second", 2, 0x1000, 0x12340000, false);
		export_entry ordinal("ordinal.dll", NULL, 65535, 0x1000, 0x7fff12340000ULL, true);
		destination.add_export(existing.address, &existing);
		source.add_export(duplicate.address, &duplicate);
		source.add_export(ordinal.address, &ordinal);
		const auto* stable = source.lookup(ordinal.address);
		destination.take_exports(source);
		require(destination.lookup(ordinal.address) == stable, "export ownership transfer copied or lost the entry");
		require(strcmp(destination.lookup(existing.address)->name, "First") == 0, "duplicate export precedence changed");
		require(source.lookup(existing.address) == NULL && source.lookup(ordinal.address) == NULL,
			"transferred exports retained stale source addresses");
		source.add_export(duplicate.address, &duplicate);
		require(source.lookup(duplicate.address) != NULL, "transferred-from export list cannot be reused");
		destination.take_exports(destination);
		require(destination.lookup(ordinal.address) == stable, "self-transfer invalidated exports");
		export_list copied;
		copied.add_exports(&destination);
		require(copied.lookup(ordinal.address) != stable && copied.lookup(ordinal.address)->ord == 65535,
			"copying export API lost independent ownership");
	}

	void entrypoint_snapshots()
	{
		temporary_file clean, ep, short_ep;
		std::shared_ptr<const pe_hash_database::entrypoint_hashes> saved;
		{
			pe_hash_database database(clean.path, ep.path, short_ep.path);
			require(database.snapshot_entrypoints()->short_hashes.empty(), "empty snapshot is not empty");
			database.add_hashes_eps({1, _UI64_MAX}, {1, _UI64_MAX});
			saved = database.snapshot_entrypoints();
			require(database.snapshot_entrypoints() == saved, "unchanged entrypoint sets were copied again");
			database.add_hashes_eps({1}, {1});
			database.add_hashes({3});
			require(database.snapshot_entrypoints() == saved, "unrelated/duplicate update invalidated the entrypoint snapshot");
			database.clear_database();
			require(database.snapshot_entrypoints()->short_hashes.empty(), "clear left a stale entrypoint snapshot");
			require(saved->minimum == 1 && saved->maximum == _UI64_MAX && saved->full.size() == 2 &&
				saved->short_hashes.size() == 2, "entrypoint snapshot does not own its hashes");
			std::atomic<bool> done(false);
			std::thread writer([&] {
				for (unsigned int i = 1; i <= 1000; ++i)
					database.add_hashes_eps({i}, {i});
				done = true;
			});
			bool valid = true;
			do
			{
				const auto snapshot = database.snapshot_entrypoints();
				valid = valid && snapshot->full == snapshot->short_hashes;
			} while (!done);
			writer.join();
			require(valid, "snapshot observed a partially published entrypoint pair");
		}
		require(saved->short_hashes.count(_UI64_MAX) == 1, "snapshot depended on database lifetime");
	}

	void snapshot_file_update()
	{
		temporary_file source, clean, ep, short_ep;
		auto bytes = pe_fixture(true);
		bytes[0x200] = 0x31;
		bytes[0x201] = 0xc0;
		source.write(bytes);
		pe_hash_database database(clean.path, ep.path, short_ep.path);
		const auto before = database.snapshot_entrypoints();
		require(database.add_file(source.path), "fixture database insertion failed");
		const auto after = database.snapshot_entrypoints();
		require(before != after && before->full.empty() && before->short_hashes.empty() &&
			after->full.size() == 1 && after->short_hashes.size() == 1, "file insertion left stale entrypoint hashes");
	}

	template<typename NT>
	void entrypoint_recovery(bool win64, bool strong)
	{
		auto bytes = pe_fixture(win64);
		auto* nt = reinterpret_cast<NT*>(bytes.data() + 0x80);
		memset(bytes.data() + 0x200, 0xcc, 0x1000);
		memset(bytes.data() + 0x240, 0x90, 8);
		bytes[0x240] = 0x31;
		bytes[0x241] = 0xc0;
		bytes[0x248] = 0xc3;
		memset(bytes.data() + 0x280, 0x90, 8);
		bytes[0x280] = 0x31;
		bytes[0x281] = 0xc0;
		bytes[0x288] = 0x31;
		bytes[0x289] = 0xc0;
		bytes[0x28a] = 0xc3;
		nt->OptionalHeader.AddressOfEntryPoint = 0x1080;
		temporary_file source, clean, ep, short_ep, output;
		source.write(bytes);
		test_options options;
		options.EntryPointHash = true;
		unsigned __int64 full, prefix;
		{
			pe_header header(source.path, &options);
			require(header.process_pe_header() && header.process_sections(), "entrypoint fixture failed");
			full = header.get_hash_ep();
			prefix = header.get_hash_ep_short();
			require(full != 0 && prefix != 0, "entrypoint fixture hashes are empty");
		}
		pe_hash_database database(clean.path, ep.path, short_ep.path);
		database.add_hashes_eps(strong ? unordered_set<unsigned __int64>{full} : unordered_set<unsigned __int64>{}, {prefix});
		nt->OptionalHeader.AddressOfEntryPoint = 0;
		source.write(bytes);
		pe_header header(source.path, &options);
		require(header.process_pe_header() && header.process_sections() && header.process_disk_image(NULL, &database),
			"entrypoint recovery failed");
		require(DeleteFileA(output.path) && header.write_image(output.path), "entrypoint output failed");
		const auto dumped = output.read();
		require(reinterpret_cast<const NT*>(dumped.data() + 0x80)->OptionalHeader.AddressOfEntryPoint ==
			(strong ? 0x1080 : 0x1040), "entrypoint weak/strong candidate precedence changed");
	}

	void borrowed_process_identity()
	{
		std::unique_ptr<void, decltype(&CloseHandle)> process(
			OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId()), CloseHandle);
		require(process != NULL, "self process handle failed");
		module_list modules(GetCurrentProcessId());
		void* base = GetModuleHandleW(NULL);
		test_options options;
		{
			pe_header owning(GetCurrentProcessId(), base, &modules, &options);
			pe_header borrowed(process.get(), base, &modules, &options);
			require(strcmp(owning.get_name(), borrowed.get_name()) == 0 &&
				strcmp(borrowed.get_name(), "hiddenmodule") != 0, "borrowed handle changed module identity");
		}
		WORD signature = 0;
		SIZE_T read = 0;
		require(ReadProcessMemory(process.get(), base, &signature, sizeof(signature), &read) &&
			read == sizeof(signature) && signature == IMAGE_DOS_SIGNATURE, "borrowed process handle was closed");
	}
}

void append_performance_tests(std::vector<test_case>& tests)
{
	tests.push_back({"pe32-import-storage-equivalence", [] { import_storage_equivalence(false); }});
	tests.push_back({"pe64-import-storage-equivalence", [] { import_storage_equivalence(true); }});
	tests.push_back({"export-ownership-transfer", export_transfer});
	tests.push_back({"entrypoint-snapshot-consistency", entrypoint_snapshots});
	tests.push_back({"entrypoint-snapshot-file-update", snapshot_file_update});
	tests.push_back({"pe32-weak-entrypoint-recovery", [] { entrypoint_recovery<IMAGE_NT_HEADERS32>(false, false); }});
	tests.push_back({"pe64-weak-entrypoint-recovery", [] { entrypoint_recovery<IMAGE_NT_HEADERS64>(true, false); }});
	tests.push_back({"pe32-strong-entrypoint-recovery", [] { entrypoint_recovery<IMAGE_NT_HEADERS32>(false, true); }});
	tests.push_back({"pe64-strong-entrypoint-recovery", [] { entrypoint_recovery<IMAGE_NT_HEADERS64>(true, true); }});
	tests.push_back({"borrowed-process-handle-identity", borrowed_process_identity});
}
