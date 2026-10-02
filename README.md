# Process Dump
Process Dump is a Windows reverse-engineering command-line tool to dump malware memory components back to disk for analysis. Often malware files are packed and obfuscated before they are executed in order to avoid AV scanners, however when these files are executed they will often unpack or inject a clean version of the malware code in memory. A common task for malware researchers when analyzing malware is to dump this unpacked code back from memory to disk for scanning with AV products or for analysis with static analysis tools such as IDA.

Process Dump works for Windows 32 and 64 bit operating systems and can dump memory components from specific processes or from all processes currently running. Process Dump supports creation and use of a clean-hash database, so that dumping of all the clean files such as kernel32.dll can be skipped. It's main features include:
* Dumps code from a specific process or all processes.
* Finds and dumps hidden modules that are not properly loaded in processes.
* Finds and dumps loose code chunks even if they aren't associated with a PE file. It builds a PE header and import table for the chunks.
* Reconstructs imports using an aggressive approach.
* Can run in close dump monitor mode ('-closemon'), where processes will be paused and dumped just before they terminate.
* Multi-threaded, so when you are dumping all running processes it will go pretty quickly.
* Can generate a clean hash database. Generate this before a machine is infected with malware so Process Dump will only dump the new malicious malware components.

I'm maintaining an official compiled release on my website here:
  https://split-code.com/processdump.html

# Installation
You can download the latest compiled release of Process Dump here:
* https://github.com/glmcdona/Process-Dump/releases

# Compiling source code
Open `pd.sln` in Visual Studio 2019 or 2022 with the Desktop development with C++ workload and a Windows 10/11 SDK installed. The solution builds the application and a dependency-free unit-test executable, using v142 on VS2019 and v143 on VS2022. Debug and Release builds are supported on Win32 and x64.

From a Visual Studio Developer PowerShell:

```powershell
msbuild pd.sln /m /p:Configuration=Release /p:Platform=x64
.\x64\Release\pd_tests.exe
```

Use `Platform=Win32` and `.\Win32\Release\pd_tests.exe` for 32-bit builds, or `Configuration=Debug` and the corresponding `Debug` directory. Tests use generated benign PE fixtures and this test process only; no administrator privileges, malware, or system-wide dumping are required. Failures exit nonzero and CRT assertions are reported to stderr instead of opening a blocking dialog.

The `--baseline` selector runs the compatibility suite. Non-reconstructed PE dumps and import-table serialization retain the fingerprints captured before the security fixes. Import-reconstructed fixtures now expect a separate `.pdimp` section rather than extension of the last section; their CRC32s and structural hashes were intentionally updated for that layout change. Expectations are checked on every run. A single case can also be selected by its printed name.

CI builds and runs tests for all four Debug/Release and Win32/x64 combinations on Windows. Development, tagged, and manually selected-commit releases reuse this tested build; release publication is separate from read-only PR builds. Compiler stack checks, Control Flow Guard, ASLR, and DEP are explicitly enabled.

## Live reconstruction benchmarks

`tests\benchmark_dumps.py` is an **opt-in** Windows benchmark using Python 3's standard library and Windows PowerShell. It snapshots running processes and dumps each accessible **main executable**, not every DLL or private allocation. It does not elevate, inject, suspend, hook, or terminate target processes. Unavailable/protected processes, exited/replaced PIDs, missing original files, and images over the 256 MiB limit are reported separately, not counted as successful tests.

Keep a baseline build, then run both builds against the same inventory:

```powershell
python .\tests\benchmark_dumps.py --exe C:\bench\before\pd.exe --report C:\bench\before.json
python .\tests\benchmark_dumps.py --exe .\x64\Release\pd.exe --snapshot C:\bench\before.json --compare C:\bench\before.json --report C:\bench\after.json
```

Repeat with `--imports` on **both** commands and distinct report names to exercise aggressive import reconstruction. Use `--pid 1234` (repeatable) or `--name spotify` to narrow a run. Each dumper child has a 30-second timeout, configurable with `--timeout`; a timeout terminates only that child. Dump files are deleted after each comparison, including failed runs. A free-space floor prevents further work below 512 MiB. Reports are never overwritten.

Security software may quarantine reconstructed executables. The benchmark distinguishes unreadable output (`analysis_unavailable`) from invalid PE data (`invalid_dump`); neither counts as success. Do not disable protection to make a benchmark pass. Use `--skip-pid 1234` to record an explicit exclusion when repeating a known-blocked target.

Reports record file sizes, zero-byte fractions, SHA-256 identities, section layout validity, 4 KiB executable-code block matches/losses, elapsed dumping time, and diagnostics. Paired comparisons require the same PID, creation time, path, original-file hash, and import mode. These are live, non-atomic snapshots: relocations, resolved imports, runtime patches, discarded pages, file overlays/signatures, and process exits prevent exact disk/memory equality. Timings include setup/export scanning and are diagnostic measurements, not controlled CPU-performance claims. Suspect flags are investigation leads, not proof of corruption.

For **actual section-content fidelity**, add `--quality --memory-witness --unique-originals` to both commands. `--unique-originals` selects one process per executable path from the inventory; it does not claim to cover every process instance. `--quality` compares bytes at corresponding section RVAs, computes normalized per-section hashes, counts nonzero-to-zero changes, compares resource payload hashes (not just directory headers), and inspects import identities and IAT contents. HIGHLOW/DIR64 base relocations and intentionally rewritten debug file offsets are normalized; IAT changes are measured separately. Other relocation types are reported, not guessed. Sections without a matching name/RVA are reported as unmatched, never synthesized as all-zero comparisons. Differences in loaded versus on-disk layout make that reference inconclusive, for example after an application update or runtime image transformation. Paired IAT summaries exclude mismatched layouts; memory summaries report how many pairs have independent witnesses on both sides. Analyzer limits/errors produce `quality_unavailable`, not a claim that the dump is corrupt.

`--memory-witness` additionally reads section contents independently, one page at a time, before and after the dump. It reports stable bytes that differ from the reconstruction separately from bytes that changed between those two reads. It excludes the IAT/debug fields intentionally rewritten by reconstruction and reports unreadable pages. A byte that changes and changes back between snapshots cannot be detected, so this is evidence of fidelity, not an atomic snapshot guarantee. Witness reads cover original section virtual extents, including initialized data beyond original raw file sizes; they can increase working set and analysis time. No memory bytes are included in reports.

Repeated imports are intentional: separate references to the same function retain independent `FirstThunk` fixups so the loader can repair every location. The analyzer's repeated-slot counts are informational, not a deduplication request or failure criterion. Native tests exercise loader-style rebinding of adjacent repeated references alongside an original IAT. Existing import tables are restored using full PE32/PE64 pointer widths, including terminators; missing lookup tables retain captured bytes, and truncated/overlapping tables are diagnosed without partial rewriting.

Reports contain local process names/paths and diagnostics, but no memory payloads. Keep them private and outside the repository. The analyzer's own portable tests run in CI via `python -m unittest discover -s tests -p test_benchmark_dumps.py`; CI never runs the live sweep.

Reconstruction preserves virtual section sizes and image extent while omitting zero-filled raw tails; initialized runtime data is retained even beyond the original disk section size. Raw sizes and offsets honor file alignment. Reconstructed imports use `.pdimp` when unused section-header space exists; otherwise the last section is explicitly extended with a warning, which can still materialize a large virtual gap. Unmapped certificate references and stale checksums are cleared, and debug payload file offsets are relocated. These dumps are for analysis, not guaranteed runnable or signed copies of the originals.

Section parsing honors extended optional-header sizes rather than assuming that the section table immediately follows the standard optional header. Export discovery includes ordinal-only and page-aligned exports, excludes holes and forwarder-string addresses, and rejects malformed table metadata without publishing partial results. Loaded export addresses redirected outside the module are preserved (as used by WOW64 USER32). Forwarded calls can still be recognized through the destination module's exports; forwarder strings themselves are not resolved as code. Consequently, aggressive-import counts may change without changing the captured section bytes or removing intentional per-location fixups.

Address-specific dumping of a region without a PE header (or with `-g`) generates both PE32 and PE64 analysis images using the same bounded reconstruction path as loose code chunks. Generated images default to EXE; the internal DLL option now sets the DLL flag correctly. Import summaries include complete pointers at the end of the captured range, count their libraries, and distinguish ordinal identities. Ordinary PE/import serialization fingerprints remain unchanged, but affected loose-code import hashes can change; regenerate a clean database when relying on those hashes. Module enumeration resizes its buffer for large processes and reports failure if a changing list cannot be captured within eight attempts (at most 1,048,576 module handles). Concurrent clean-database lookups and updates are synchronized, and database-generation options have deterministic defaults.

Native regression cases enforce a maximum 12 KiB dump (16 KiB with imports) for a fixture containing a 4 MiB mostly-zero virtual data section. They also cover valid sections beyond the former 60,000 KiB truncation threshold, sparse RVAs, unaligned image ends, high-address/terminal imports, occupied section-header space, and byte-preserving reloads of 32 varied PE32/PE64 layouts.

## Controlled performance benchmarks

`pd_tests.exe --benchmark imports 4 8` runs eight reconstruction jobs on four workers. The opt-in workloads are `dense` (4 MiB), `sparse` (32 MiB with a 4 KiB initialized prefix), `imports` (65,536 independent fixups), `entrypoint` (a nonmatching entrypoint database), `entrypoint-empty`, `database` (known-hit/miss queries), and `exports` (discovery in the test process itself). Fixtures are never executed. No other process is inspected or modified.

Save a reference Release test executable **with the same benchmark harness**, then alternate reference/candidate trials:

```powershell
python .\tests\benchmark_performance.py --baseline C:\bench\before\pd_tests.exe --exe .\x64\Release\pd_tests.exe --report C:\bench\performance.json --threads 1 2 4 8 --jobs 16 --repeats 5
```

Reports retain individual samples, median phase timings, ratios, executable hashes, worker CPU cycles, coarse thread CPU times, and process peak working set. `capture_ms`, `reconstruct_ms`, `write_ms`, `cleanup_ms`, and `service_ms` sum work across jobs; they are **not** parallel elapsed time. `worker_ms` ends when the last worker finishes processing; `wall_ms` also includes thread teardown/join. Fixture setup, output-name reservation, thread creation, output readback/CRC, and file deletion are outside these intervals. A warmup precedes every trial. Peak working set includes warmup but is sampled before final readback. Timing still reflects filesystem caching, security scanning, CPU scheduling/frequency, and memory-bandwidth contention; worker throughput and maximum thread count are not universal speedup guarantees.

Every serialized job is checked after timing against the warmup's output size and CRC32, normalizing only the fixture's ASLR-dependent PE64 image base. The runner rejects changed fingerprints or mismatched configurations across trials/builds. Database queries verify their expected hit count; export discovery has no serialized output, so neither reports output-fingerprint equivalence. Use the native semantic tests and the separate live-content benchmark for those paths. Temporary fixture outputs are removed on normal/error unwinding; forcibly killing the benchmark can leave files behind.

For allocation attribution, build `tests\pd_tests.vcxproj` with `/p:ProfileAllocations=true` and separate `OutDir`/`IntDir` directories. This replaces C++ `new`/`delete` in the **test executable only**, counting thread-local calls/requested bytes during each job, not every CRT/Windows allocation. Compare instrumented builds only to other instrumented builds, and confirm timings separately with ordinary Release builds. Allocation instrumentation cannot be combined with AddressSanitizer. Reports are local-only, never overwritten, and not uploaded automatically.

The optimized pipeline keeps captured bytes read-only during packing, scans zero tails a machine word at a time, and avoids a whole-image copy when imports are disabled. Import records are contiguous; reconstruction borrows names from its immutable export list instead of allocating copies for each fixup. Standalone string-based import APIs still own their strings. Export-list transfers retain first-wins alias precedence and stable record addresses. Process capture reuses the dumper's handle while preserving module names and ownership. When entrypoint recovery is needed, each image acquires a consistent immutable snapshot of both entrypoint sets, skips empty databases, and scans without contending on the shared database lock. Workers share the cached snapshot until entrypoint data changes, rather than copying a potentially large database per image. Updates become visible to the next snapshot; ordinary clean-hash lookups remain synchronized. Repeated per-location imports and PE/import serialization semantics are unchanged.

## Handling untrusted input

PE headers, sections, imports, and exports are treated as untrusted data. Invalid ranges and reconstruction sizes above 256 MiB per image are rejected with a diagnostic instead of attempting unsafe allocations. This limit also applies to generated headers and reconstructed disk images.

Process and module names are encoded as single filename components. Dump files are **created only if the path does not exist**: existing dumps, hard links, and symbolic links are never overwritten. Use a fresh output directory when repeating a dump. Output directories containing reparse points (including junctions), unsupported device paths, and names exceeding supported Windows path/component lengths are rejected. Use an ordinary directory you control, outside the target's writable directories.

Close monitoring uses a private release event rather than accepting a target-supplied thread ID. Stopping monitoring restores the original entry bytes and protection, then releases callbacks even if they have not started waiting yet. A published callback's two-page allocation and target-side event handle remain until that target exits; reclaiming them earlier could free code beneath an in-flight thread. Unpublished allocations and handles are cleaned up on failure.

`pd_tests.exe --security` runs bounded validation and mock-based lifecycle tests; the tests contain no exploit PoCs and do not hook other processes.

# Command-line arguments
Process dump can be used to dump all unknown code from memory ('-system' flag), dump specific processes, or run in a monitoring mode that dumps all processes just before they terminate.

Before first usage of this tool, when on the clean workstation the clean excluding hash database can be generated by either:
* pd -db genquick
* pd -db gen

Example Usage:
* pd -system
* pd -pid 419
* pd -pid 0x1a3
* pd -pid 0x1a3 -a 0x401000 -o c:\dump\ -c c:\dump\test\clean.db
* pd -p chrome.exe
* pd -p "(?i).\*chrome.\*"
* pd -closemon

The command-line arguments can be grouped as follows:

**General Dumping Options**

| Option | Description |
|--------|-------------|
| -system | Dumps all modules not matching the clean hash database from all accessible processes into the working directory. |
| -pid \<pid\> | Dumps all modules not matching the clean hash database from the specified PID into the current working directory. Use a '0x' prefix to specify a hex PID. |
| -closemon | Runs in monitor mode. When any processes are terminating, process dump will first dump the process. |
| -p \<regex process name\> | Dumps all modules not matching the clean hash database from the process name found to match the filter into specified PID into the current working directory. |
| -a \<module base address\> | Dumps a module at the specified base address from the process. |
| -o \<path\> | Sets the default output root folder for dumped components. |

**Clean Hash Database Options**

| Option | Description |
|--------|-------------|
| -db gen | Automatically processes a few common folders as well as all the currently running processes and adds the found module hashes to the clean hash database. It will add all files recursively in: `%WINDIR%`, `%HOMEPATH%`, `C:\Program Files\`, `C:\Program Files (x86)\`, as well as all modules in all running processes. These clean hashes will be added to the file `clean.hashes` in the application directory. During future process dumping commands, these known modules will not be dumped. It is recommended to run this command one time on a clean system prior to using the tool that way not too many modules will be dumped from memory.|
| -db genquick | Same as above, but only adds the hashes from all modules in all processes to the clean hash database. This is a much faster way to build the clean hash database, but it will be less complete. |
| -db add \<dir\> | Adds all the files in the specified directory recursively to the clean hash database. |
| -db rem \<dir\> | Removes all the files in the specified directory recursively from the clean hash database. |
| -nr | Disable recursion on hash database directory add or remove commands. |
| -db clean | Clears the clean hash database. |
| -db ignore | Ignores the clean hash database when dumping a process this time. All modules will be dumped even if a match is found. |
| -cdb \<filepath\> | Full filepath to the clean hash database to use for this run if you'd like to override the default of `clean.hashes`. |
| -edb \<filepath\> | Full filepath to the entrypoint hash database to use for this run. |
| -esdb \<filepath\> | Full filepath to the entrypoint short hash database to use for this run. |

**Output Options**

| Option | Description |
|--------|-------------|
| -v | Verbose mode where more details will be printed for debugging. |
| -nh | No header is printed in the output. |

**Advanced Options**

| Option | Description |
|--------|-------------|
| -g | Forces generation of PE headers from scratch, ignoring existing headers. |
| -eprec | Force the entry point to be reconstructed, even if a valid one appears to exist. |
| -ni | Disable import reconstruction. |
| -nc | Disable dumping of loose code regions. |
| -nt | Disable multithreading. |
| -nep | Disable entry point hashing. |
| -t \<thread count\> | Sets the number of threads to use (default 16). |

# Usage Examples

| Command | Description |
| ------- | ----------- |
| `pd64.exe -db genquick` | Quickly build clean module database based on currently running processes. Process Dump in later tasks will only dump unrecognized modules. |
| `pd64.exe -system` | Dump all modules and hidden chunks from all processes while ignoring clean modules. |
| `pd64.exe -closemon` | Run in terminate monitor mode. This will dump all processes when they attempt to terminate. |
| `pd64.exe -pid 0x18A` | Dump modules and hidden chunks from a specific process ID. |
| `pd64.exe -p .\*chrome.\*` | Dump modules and hidden chunks by process name. |
| `pd64.exe -db gen` | Build a clean-hash database of known modules. This is used to avoid dumping known good modules in later tasks. |
| `pd64.exe -pid 0x1a3 -a 0xffb4000` | Dump code from a specific address in PID. This will generate two files for analysis, with reconstructed 32bit and 64bit PE headers: `notepad_exe_x64_hidden_FFB40000.exe` and `notepad_exe_x86_hidden_FFB40000.exe`. |

Sure, here's a more streamlined version of the information:

## Sandbox Usage

When using Process Dump in an automated sandbox or for manual anti-malware research, the following steps can be useful. Make sure to run all commands as an Administrator in a clean environment.

- **Build the Clean Hash Database:** Run `pd64.exe -db gen` or for a faster less complete process, use `pd64.exe -db genquick`. Depending on your situation, you may want to snapshot your VM after creating this clean hash database that way it doesn't need to be repeated each time.

- **Start the Process Dump Terminate Monitor:** Keep `pd64.exe -closemon` running in the background. It will dump all intermediate processes used by the malware.

- **Execute the Malware File:** Monitor the malware installation. `pd64.exe` will automatically dump any process that tries to close.

- **Dump the Running Malware from Memory:** When ready, use `pd64.exe -system` to dump all processes.

The dumped components will be found in the working directory of `pd64.exe`. To change the output path, use the `-o` flag.

# Notes on the naming convention of dumped modules:
* 'hiddemodule' in the filename instead of the module name indicates the module was not properly registered in the process.
* 'codechunk' in the filename means that it is a reconstructed dump from a loose executable region. This can be for example injected code that did not have a PE header. Codechunks will be dumped twice, once with a reconstructed x86 and again with a reconstructed x64 header.

Example filenames of dumped files
* notepad_exe_PID2990_hiddenmodule_16B8ABB0000_x86.dll
* notepad_exe_PID3b5c_notepad.exe_7FF6E6630000_x64.exe
* notepad_exe_PID2c54_codechunk_17BD0000_x86.dll
* notepad_exe_PID2c54_codechunk_17BD0000_x64.dll


# Version history

## Version 2.1 (February 12th, 2017)
* Fixed a bug where the last section in some cases would instead be filled with zeros. Thanks to megastupidmonkey for reporting this issue.
* Fixed a bug where 64-bit base addresses would be truncated to a 32-bit address. It now properly keeps the full 64-bit module base address. Thanks to megastupidmonkey for reporting this issue.
* Addressed an issue where the processes dump close monitor would crash csrss.exe.
* Stopped Process Dump from hooking it's own process in close monitor mode. 

## Version 2.0 (September 18th, 2016)
* Added new flag '-closemon' which runs Process Dump in a monitoring
   mode. It will pause and dump any process just as it closes. This is designed
   to work well with malware analysis sandboxes, to be sure to dump
   malware from memory before the malicious process closes.
*  Upgraded Process Dump to be multi-threaded. Commands that dump or get
   hashes from multiple processes will run separate threads per operation.
   Default number of threads is 16, which speeds up the general Process
   Dump dumping processing significantly.
*  Upgraded Process Dump to dump unattached code chunks found in memory.
   These are identified as executable regions in memory which are not
   attached to a module and do not have a PE header. It also requires that
   the codechunk refer to at least 2 imports to be considered valid in
   order to reduce noise. When dumped, a PE header is recreated along with
   an import table. Code chunks are fully supported by the clean hash database.
*  Added flags to control the filepath to the clean hash database as well
   as the output folder for dumped files.
*  Fix to generating clean hash database from user path that was causing a
   crash.
*  Fix to the flag '-g' that forces generation of PE headers. Before even
   if this flag was set, system dumps (-system), would ignore this flag
   when dumping a process.
* Various performance improvements.
* Upgraded project to VS2015.

## Version 1.5 (November 21st, 2015)
* Fixed bug where very large memory regions would cause Process Dump to hang.
* Fixed bug where some modules at high addresses would not be found under 64-bit Windows.
* More debug information now outputted under Verbose mode.

## Version 1.4 (April 18th, 2015)
* Added new aggressive import reconstruction approach. Now patches up all DWORDs and QWORDs in the module to the corresponding export match.
* Added '-a (address to dump)' flag to dump a specific address. It will generate PE headers and build an import table for the address.
* Added '-ni' flag to skip new import reconstruction algorithm.
* Added '-g' flag to force generation of new PE header even if there exists one when dumping a module. This is good if the PE header is malformed for example.
* Various bug fixes.

## Version 1.3 (October 10th, 2013)
* Improved handling of PE headers with sections that specify invalid virtual sizes and addresses.
* Better module dumping methodology for dumping virtual sections down to disk sections.

## Version 1.1 (April 8th, 2013)
* Fixed a compatibility issue with Windows XP.
* Corrected bug where process dump would print it is dumping a module but not actually dump it.
* Implemented the '-pid ' dump flag.

## Version 1.0 (April 2nd, 2013)
* Initial release.
