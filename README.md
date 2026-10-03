# Process Dump 3.0.0

The source version is **3.0.0** (Windows file/product version **3.0.0.0**). The console banner and executable resources share `pd\version.h`. See the [3.0.0 changes](#version-300-unreleased) and [command-line reference](#command-line-arguments). This source version does not imply that a tagged release has been published.

Process Dump is a Windows reverse-engineering command-line tool to dump malware memory components back to disk for analysis. Often malware files are packed and obfuscated before they are executed in order to avoid AV scanners, however when these files are executed they will often unpack or inject a clean version of the malware code in memory. A common task for malware researchers when analyzing malware is to dump this unpacked code back from memory to disk for scanning with AV products or for analysis with static analysis tools such as IDA.

Process Dump works for Windows 32 and 64 bit operating systems and can dump memory components from specific processes or from all processes currently running. Process Dump supports creation and use of a clean-hash database, so that dumping of known modules such as kernel32.dll can be skipped. A hash match is an exclusion, not a trust verdict. Its main features include:
* Dumps code from a specific process or all processes.
* Finds and dumps hidden modules that are not properly loaded in processes.
* Finds and dumps loose code chunks even if they aren't associated with a PE file. It builds a PE header and import table for the chunks.
* Reconstructs imports using an aggressive approach.
* Can run in close dump monitor mode ('-closemon'), where processes will be paused and dumped just before they terminate.
* Shares persistent workers across modules for system dumping and live hashing; directory database generation also hashes files concurrently.
* Recovers missing/invalid entrypoints using full opcode signatures and a conservative x64 runtime-function fallback.
* Offers opt-in `-reexec` preparation for fresh-launch experiments, separate from normal forensic capture.
* Can generate clean-module and entrypoint databases. Generate a baseline on a known-clean system to reduce repeated output; unknown code is not necessarily malicious.

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

Release jobs use SHA-pinned actions and a preparation script from the workflow's own commit, never the manually selected build commit. Artifacts are read only from the current run, with exactly one ordinary `pd.exe` entry per architecture and 64 MiB archive/binary limits. The publisher validates ZIP metadata and CRC, then copies bytes to fixed, exclusively created filenames; it never extracts archive paths or executes downloaded binaries. Signed download redirects do not receive the repository token. Run `python -m unittest discover -s tests -p "test_*.py"` for the portable benchmark, documentation and release-preparation tests. To also check the built application's help, no-side-effect help exit and Windows version metadata, set `$env:PD_TEST_EXE = (Resolve-Path .\x64\Release\pd.exe).Path` before running that command. CI sets this for each build configuration; CLI checks use a private copy and never dump or hook processes.

Release assets include `provenance.json` with source commit, run/artifact IDs, sizes, and SHA-256 hashes. Existing version tags must identify the tested commit. Publication is serialized per release; superseded automatic main builds do not replace development assets. Manual development dispatch still deliberately permits a maintainer-selected branch. `Develop` remains a rolling asset set with a historical, unmoved tag: use the manifest, not that tag, to identify its binaries. Multi-asset uploads are not transactional; a failed upload can leave a partial set, so verify both binary hashes against the manifest. Selecting a commit authorizes publication of its bytes, not execution of its build code in the write-enabled publisher.

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

## System scheduling and database generation

`-system` now shares a persistent worker pool between per-process scanning, export parsing, individual module reconstruction/hashing, and loose-code processing. Idle workers can help with the last process instead of waiting for one process-owned thread. A producer waiting for its modules helps execute only module jobs: it never recursively starts another process or creates another thread pool. At most `-t` workers execute work (`-nt` uses one). Each process retains its handle, module snapshot and immutable export table until its jobs finish. Export aliases are merged in snapshot order, not completion order. Only small job metadata is queued; images are allocated when workers execute them, with at most 256 queued leaf jobs per active producer.

The full region scan still discovers hidden/headerless code and retains the existing 1,000-page-per-region and loose-chunk limits; this is not a loaded-module-only shortcut. One region walk collects both PE candidates and executable heaps. Module exclusions finish before loose heaps are processed, retaining adjacent heaps. System dumping still updates its in-memory clean set between completed processes and performs one final new-PID scan after all initial work drains. Cross-process deduplication remains scheduling-dependent and snapshots remain non-atomic. The close-monitor and direct PID paths retain their synchronous lifecycle.

`-db genquick` uses the same module pool for live hashing. `-db gen` and `-db add` also hash files concurrently using `-t`, with bounded batches of 256 file jobs; recursion, filename filtering and reparse-directory exclusions are retained. Database hashing no longer changes the caller's import-reconstruction setting. The CLI continues to require generation and dumping as separate commands. The three existing database formats/signatures are unchanged; compare their **sets**, not serialization order. Parallelism increases simultaneous image memory and is not a guarantee that 16 workers beat four on an I/O-bound machine.

Two opt-in runners make these comparisons repeatable:

```powershell
python .\tests\benchmark_system.py --probe .\x64\Release\pd_tests.exe --report C:\bench\system.json --threads 1 4 16 --repeats 3 --acknowledge-execution
python .\tests\benchmark_database.py --baseline C:\bench\before\pd.exe --exe .\x64\Release\pd.exe --manifest C:\bench\corpus.txt --report C:\bench\database.json
```

The system runner launches only its own benign test host containing 64 inert, non-executable mapped PE buffers plus its ordinary loaded modules. It uses the production system scheduler with an explicit one-PID inventory, modelling the final-process imbalance without dumping unrelated processes. It compares serial execution with 1/4/16 workers, alternates order, requires identical filenames and SHA-256 bytes for every dump and identical module/full-entrypoint/short-entrypoint hash sets, and removes captures afterward. `--flags` selects combinations of `i` (imports), `c` (loose chunks), `g` (generated headers) and `r` (reexecution); ordinary defaults test imports/chunks on and off. Synthetic images are never executed. The database runner stages the line-separated manifest's files privately, executes only the supplied dumper builds, and rejects missing, truncated or changed database sets. Reports are never overwritten; keep their local paths/hashes private. Run timing benchmarks separately from other builds/profilers.

On the local 64-module tail workload, three alternating isolated trials of the final build gave median dump-plus-hash times of **523.02 ms serial / 342.58 ms with four workers / 320.87 ms with 16** for imports and loose chunks enabled. With both disabled, the corresponding times were **373.07 / 183.16 / 165.31 ms**. Live hashing with chunks and imports enabled improved **119.37 / 70.08 / 67.29 ms**. All 96 final timed runs preserved exact dump bytes and hash sets. An earlier 96-run measurement, another 24 runs covering generated-header/reexecution combinations, and a six-run Win32 smoke comparison also retained equivalence. All eight option combinations retained byte-identical output against the saved pre-change dumper (78 normal or 156 generated images). This measures a deliberately imbalanced tail, not a universal full-system speedup.

For 621 identical staged PE files, three alternating trials of database generation produced median elapsed times of **500.65 ms before / 478.35 ms with one worker / 240.28 ms with four / 221.62 ms with 16**, with all three hash sets identical. The first baseline trial was cold and took 10.94 seconds; it is retained in the report rather than misrepresented as a 50-fold CPU improvement. Filesystem and security-scanner caching still influence these short runs.

A separate real-machine `-db genquick -t 4 -nh` run completed with 3,237 clean hashes, 484 full entrypoint hashes and 121 short hashes. A subsequent default `-system -t 4` run using those private databases completed with 507 outputs (about 231 MiB), retaining imports and loose chunks. Captures were removed after recording their hashes. Existing access restrictions and the 256 MiB per-image bound still apply. Full `-db gen` traversal over all Windows/user/program folders was not timed end-to-end: its live-hashing component, recursive file worker, filters and all three output sets were exercised separately.

## Entrypoint recovery

Automatic recovery preserves an existing entrypoint in executable section storage, including legitimate RVA `0x2000` and low-alignment images. `-eprec` still forces a search; `-nep` disables it. Recovery scans executable section ranges rather than every image byte, requires a full opcode-hash match, and preserves the earliest exact-prefix/full-hash match regardless of verbosity. An eight-byte prefix alone is no longer enough to rewrite the entrypoint. If exact-prefix matching fails on x64, validated exception/unwind-table function starts provide a bounded fallback: only a unique full opcode-hash match is selected. Malformed or ambiguous metadata is not guessed. No original file, PDB or external symbol service is required.

The read-only `pd_tests.exe --entrypoint-corpus C:\bench\corpus.txt C:\bench\recovery.csv` benchmark evaluates known on-disk entrypoints without executing the images. It compares seven policies: legacy weak/strong selection, executable-only strong selection, unique strong matches, function-boundary fallback, unique weak fallback, exact-match precedence, and x64 runtime-function-only fallback. It also invokes the production recovery method with the header entrypoint erased. Both an oracle database containing the image and a leave-one-image-out database are measured; the latter retains signatures shared by other files and is not a leave-one-family-out generalization claim.

A SHA-256-deduplicated local corpus selected 621 system/application images (about 551 MiB); 577 had a nonzero executable entrypoint and were measured (263 PE32, 314 PE64). With each image's own signature contribution withheld, legacy recovery returned **323 correct / 252 wrong / 2 unresolved** results. The selected production policy returned **357 correct / 11 wrong / 209 unresolved**. With the image represented in the database, both recovered all **577/577**. Exact-only recovery produced 316 correct / 7 wrong / 254 unresolved held-out results; the selected runtime-function fallback adds 41 correct recoveries at the cost of four additional wrong guesses. Requiring uniqueness for every exact match unnecessarily lost seven known-image recoveries; scanning x86 call targets added no correct recoveries, so that extra work was omitted. Retaining weak-only guesses sharply increased false positives. These measurements support conservative abstention, not a claim of perfect recovery: opcode fingerprints can still collide and unusual/packed code or absent training signatures can remain unresolved. Unsuccessful forced recovery leaves the prior RVA unchanged and reports it.

## Experimental re-execution

`-reexec` prepares an analysis dump for a **fresh launch**, rather than preserving all captured state. It is off by default and never executes a dump itself. It restores the compiler's GS-cookie bootstrap value through validated load-config metadata, resets writable/non-executable zero-fill tails described by the captured section headers, and limits **new speculative** imports to originally file-backed writable/non-executable data outside delay-IAT sections. Existing import reconstruction and independent repeated eligible fixups remain supported. ASLR, DEP, CFG and delay-IAT protections are not disabled.

These changes deliberately discard runtime data. They assume the section headers still describe the original on-disk layout; do not use this mode when faithful analysis of unpacked or runtime-generated data is the goal. Initialized file-backed globals, heaps, handles, TLS, dependencies, resources and external state are not restored. This is not a process checkpoint/restart facility or a guarantee that a dumped application will run. Normal dumping continues to preserve initialized virtual tails and aggressive import coverage.

`tests\benchmark_reexecution.py` measures explicitly selected **trusted, benign** applications using a same-architecture `pd_tests.exe` debugger probe. It launches the original, an untouched staged disk-copy control, and the reconstruction with the same arguments and working directory. Only its newly created children are debugged and assigned to a kill-on-close job; unrelated running applications are untouched. A temporary entrypoint breakpoint is restored, including the instruction pointer, before capture. `--phase entry` captures at that breakpoint, `--phase ready` at the test host's `PD_REEXEC_READY` marker, and `--phase idle` after GUI input-idle. Entry/ready captures occur while a debug event suspends the child; idle capture is live and non-atomic. Dumping time is excluded from the observation duration.

For example, retain a baseline `pd.exe`, create a private `C:\bench` directory, then compare Notepad startup on a machine with this matching language resource:

```powershell
python .\tests\benchmark_reexecution.py --app C:\Windows\System32\notepad.exe --probe .\x64\Release\pd_tests.exe --dumper C:\bench\before\pd.exe --mui C:\Windows\System32\en-US\notepad.exe.mui --imports --report C:\bench\notepad-before.json --acknowledge-execution
python .\tests\benchmark_reexecution.py --app C:\Windows\System32\notepad.exe --probe .\x64\Release\pd_tests.exe --dumper .\x64\Release\pd.exe --mui C:\Windows\System32\en-US\notepad.exe.mui --imports --prepare --compare C:\bench\notepad-before.json --report C:\bench\notepad-after.json --acknowledge-execution
```

Omit `--imports` on both commands to disable additional import discovery. `--prepare` passes `-reexec`; `--arg=VALUE` is repeatable. Use `--arg=--reexecution-fixture --phase ready` with a frozen copy of `pd_tests.exe` as the application to exercise initialized CRT state, a worker thread and normal completion. `--milliseconds` controls the 100..30,000 ms observation window (default 3,000); each dumper has a separate 30-second deadline. Optional `--loader-snaps` collects bounded loader diagnostics only in the owned child, without changing system-wide settings. Both reports must use the same probe build, application, arguments, resources, capture phase, duration and OS; changed or failed controls make a comparison inconclusive. Controls must reach their entrypoint and either GUI idle or exit zero without timing out; intentional nonzero console exits also require manual interpretation rather than an automatic comparison.

The optional `--mui` copies only the explicitly supplied matching `locale\app.exe.mui` beside both staged launches; no original executable code is substituted into the dump. Missing resource/dependency files and changed application directories can prevent an otherwise valid image from starting, which is why the staged original is a separate control. Reports distinguish loader, entrypoint, GUI input-idle, fixture thread/completion and clean-exit milestones, and compare captured console-output hashes. GUI input-idle proves startup progress, not editor functionality; exit zero alone does not prove GUI or fixture completion. Application hashes, module-relative exception locations and exit/exception codes are recorded.

On Windows 11 build 22631, two trials of six frozen/system application images, both import modes and the relevant capture phases produced **40 matched baseline/prepared pairs**: 34 gained execution milestones, six retained their previous progress, and none lost milestones. Both x86/x64 test hosts completed their threaded fixture from entrypoint and initialized-CRT captures (16 runs). `sort.exe` produced the expected sorted bytes and `findstr.exe /?` the same help output as both controls (eight runs). Both x86/x64 Notepads captured at entrypoint reached GUI input-idle with the matching MUI (eight runs). Notepad captured at GUI idle progressed past its former loader rejection but still failed after entrypoint with `0xc0000409`, parameter 5 (eight runs). These are bounded startup observations, not general restart guarantees; the remaining failure was not worked around by resetting arbitrary initialized globals or weakening mitigations.

This harness is **not a security sandbox**: children run with the caller's permissions and can affect files, the network and external applications. Never select malware or unknown images, bypass security software, or supply destructive application arguments. Output redirection is not size-limited; select bounded workloads. The normal unit suite never executes its synthetic PE fixtures, and CI never runs this opt-in launch benchmark. Temporary captures/staged copies are removed on normal/error unwinding; forced termination can leave files behind. Reports contain local paths, addresses and diagnostics, but no captured memory payloads; keep reports and any investigation dumps private and outside the repository.

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

Process and module names are encoded as single filename components. Dump files are **created only if the path does not exist**: existing dumps, hard links, and symbolic links are never overwritten. Use a fresh output directory when repeating a dump. Output directories containing reparse points (including junctions), unsupported device paths, and names exceeding supported Windows path/component lengths are rejected. Creation uses `NtCreateFile` with `FILE_CREATE` and `OBJ_DONT_REPARSE`, relative to the held root directory, so intermediate reparses are rejected during creation even if directory metadata changes after validation. Directory handles also prevent rename/replacement during writing. Failure of the safe creation operation is reported without retrying through a weaker path API. Use an ordinary directory you control, outside the target's writable directories.

Close monitoring uses a private release event rather than accepting a target-supplied thread ID. Stopping monitoring restores the original entry bytes and protection, then releases callbacks even if they have not started waiting yet. A published callback's two-page allocation and target-side event handle remain until that target exits; reclaiming them earlier could free code beneath an in-flight thread. Unpublished allocations and handles are cleaned up on failure.

`pd_tests.exe --security` runs bounded validation and mock-based lifecycle tests; the tests contain no exploit PoCs and do not hook other processes.

# Command-line arguments
Use `pd.exe <command> [options]`. Local builds are named `pd.exe`; release assets are `pd64.exe` and `pd32.exe`. Prefer the 64-bit build on 64-bit Windows to access both architectures. Administrator rights improve coverage but do not bypass protected processes.

Choose one dump or database-maintenance command per invocation; generate the baseline and dump in **separate commands**. `-db ignore` is a modifier that can accompany dumping. Flags are case-sensitive. No arguments, `--help`, `-help`, `-h` or `--h` print help and exit without running a dump or database command.

Defaults: aggressive import reconstruction, loose-code discovery, and entrypoint recovery are enabled; `-reexec`, generated headers and verbose output are disabled. Dumps go to the current working directory; database files live beside the executable. The default worker count is 16.

**Dump commands and output**

| Option | Description |
|--------|-------------|
| `-system` | Dump unknown modules, hidden modules and eligible loose chunks from all accessible processes. Workers share module jobs, including the last process; one final scan discovers new PIDs after initial work drains. |
| `-pid <pid>` | Dump one process. Use decimal or a `0x`-prefixed hexadecimal PID. |
| `-p <regex>` | Match the **entire** process name using a case-sensitive ECMAScript regular expression. Quote the expression; use `.*` for a substring match. Multiple matches prompt for approval. Inline `(?i)` is not supported; use explicit character classes when needed. |
| `-closemon` | Hook termination of accessible processes and dump them before exit. This modifies target processes; use only in a controlled environment. Press Ctrl+C to stop and clean up hooks. It cannot guarantee capture of every termination path or protected process. |
| `-a <address>` | Dump at a decimal or `0x`-prefixed base address; **requires `-pid`**. Reuse a valid PE header. If absent, or with `-g`, generate both PE32 and PE64 analysis images. Clean filtering still applies. |
| `-o <path>` | Output directory, defaulting to the current working directory. Create an ordinary directory you control first. Existing files are never overwritten and reparse paths are rejected; use fresh directories for repeat captures. |

**Database commands and paths**

There are three databases: clean-module hashes (`clean.hashes`), full entrypoint-opcode hashes (`entrypoints.hashes`), and short entrypoint-prefix hashes (`shortentrypoints.hashes`). Defaults are beside the executable, not in the working/output directory. Maintenance commands save to these paths; use all three path overrides for an isolated baseline. Generation/addition extends existing sets rather than replacing them. Generate only on a known-clean system; skipped matches do not prove that all remaining code is malicious.

| Option | Description |
|--------|-------------|
| `-db gen` | Add live module/chunk and entrypoint hashes, then recursively scan PE files in `%WINDIR%`, `%USERPROFILE%`, `C:\Program Files` and `C:\Program Files (x86)`. Uses `-t` for live and file work. `-nr` does not change this full recursive scan. |
| `-db genquick` | Add live module/chunk and entrypoint hashes only, without scanning disk folders. Uses the shared module pool; `-nc` excludes loose chunks. Faster but less complete than `gen`. |
| `-db add <dir>` | Add PE file hashes from a directory to all three databases, recursively by default. Uses `-t`; reparse directories are excluded. |
| `-db remove <dir>`, `-db rem <dir>` | Remove matching **clean-module** hashes for PE files in the directory, recursively by default. Does not remove entrypoint signatures. Removal remains serial. |
| `-nr` | Disable recursion for `-db add`, `-db remove` and `-db rem` only. |
| `-db clean` | Clear and save **all three databases**, including both entrypoint sets. |
| `-db ignore` | Ignore **all three loaded sets** in memory for this dump invocation, including entrypoint signatures; does not clear database files. In-run cross-process deduplication can still skip repeated modules. |
| `-cdb <filepath>`, `-c <filepath>` | Override the clean-module database path. |
| `-edb <filepath>` | Override the full entrypoint-opcode database path. |
| `-esdb <filepath>` | Override the short entrypoint-prefix database path. |

**Reconstruction and workers**

| Option | Description |
|--------|-------------|
| `-g` | Force generated PE32 and PE64 analysis headers instead of existing headers. Both architectures are emitted because raw code does not reliably identify its architecture. |
| `-ni` | Disable aggressive import reconstruction (enabled by default). |
| `-nc` | Disable loose-code dumping and hashing (enabled by default). Loaded and hidden PE modules are still scanned. |
| `-nep` | Disable entrypoint recovery and live entrypoint-signature collection; takes precedence over `-eprec`. File database add/gen still collect entrypoint signatures using their own parsing options. |
| `-eprec` | Force entrypoint recovery even when a valid executable entrypoint exists. Normally valid entrypoints are preserved. Requires full opcode matches; x64 runtime-function metadata provides a fallback. Weak prefixes alone are not accepted, and failure retains the previous RVA with a warning. Recovery is heuristic, not guaranteed. |
| `-reexec` | Experimental fresh-launch preparation, off by default. Reset the GS-cookie bootstrap value and writable/non-executable zero-fill tails; restrict new imports to file-backed writable data outside delay-IAT sections. Assumes original section layout and **discards captured state**. Does not execute dumps, restore a process checkpoint or disable ASLR/DEP/CFG. See [re-execution caveats](#experimental-re-execution). |
| `-t <count>` | Set worker count to at least 1 (default 16; decimal or `0x`-prefixed hex). Applies to system dumping, live/file database generation and close-monitor dump workers. Direct `-pid`/`-p` dumps remain synchronous. More workers can increase memory use and are not always faster. |
| `-nt` | Use one worker, equivalent to `-t 1`; the last `-t`/`-nt` wins. Close monitoring still has a separate monitoring thread. |

**Help and diagnostics**

| Option | Description |
|--------|-------------|
| `--help`, `-help`, `-h`, `--h` | Print help and exit. No arguments also prints help. |
| `-v` | Enable verbose diagnostics (off by default). |
| `-nh` | Suppress the version/copyright banner, not other output; help text still prints. |

Images and reconstructions above 256 MiB are rejected. Captures are live and non-atomic, can contain sensitive data, and are not guaranteed runnable or signed copies. Keep output private and review the [input/output safety notes](#handling-untrusted-input).

# Usage Examples

| Command | Description |
| ------- | ----------- |
| `pd64.exe -db genquick -t 4` | Build a live baseline on a known-clean system. Run separately from dumping. |
| `pd64.exe -system -t 4 -o C:\dumps` | Dump unknown modules/chunks across accessible processes into a pre-created output directory. |
| `pd64.exe -closemon -o C:\dumps` | Monitor hooked termination paths in a controlled environment; Ctrl+C stops monitoring. |
| `pd64.exe -pid 0x18A` | Dump modules and eligible loose chunks from one process. |
| `pd64.exe -p ".*chrome.*" -ni -nc` | Match process names containing lowercase `chrome`, without aggressive imports or loose chunks. |
| `pd64.exe -p "chrome[.]exe"` | Match exactly `chrome.exe`. |
| `pd64.exe -db gen` | Extend the live and disk baseline using the documented folders. |
| `pd64.exe -db add C:\baseline -nr -t 4` | Add PE files in just this directory with four workers. |
| `pd64.exe -pid 0x1a3 -a 0x401000 -o C:\dumps` | Dump at a chosen base address. Generates both architectures only when a header is absent or `-g` is supplied. |
| `pd64.exe -pid 419 -reexec -o C:\dumps` | Prepare dumps for fresh-launch experiments; does not launch them. |
| `pd64.exe -pid 419 -eprec -cdb C:\baseline\clean.hashes -edb C:\baseline\entrypoints.hashes -esdb C:\baseline\shortentrypoints.hashes` | Force recovery using an isolated set of database paths. |

## Sandbox Usage

When using Process Dump in an automated sandbox or for manual anti-malware research, the following steps can be useful. Make sure to run all commands as an Administrator in a clean environment.

- **Build the Clean Hash Database:** Run `pd64.exe -db gen` or for a faster less complete process, use `pd64.exe -db genquick`. Depending on your situation, you may want to snapshot your VM after creating this clean hash database that way it doesn't need to be repeated each time.

- **Start the Process Dump Terminate Monitor:** Keep `pd64.exe -closemon` running in the background. It attempts to capture accessible processes using hooked termination paths; capture of every intermediate process is not guaranteed.

- **Observe the Sample in an Isolated Lab:** Monitor the sample's behavior while `pd64.exe` captures hooked processes that terminate. Never use close monitoring as a containment or safety mechanism.

- **Dump the Running Malware from Memory:** When ready, use `pd64.exe -system` to dump all processes.

The dumped components will be found in the working directory of `pd64.exe`. To change the output path, use the `-o` flag.

# Notes on the naming convention of dumped modules:
* 'hiddenmodule' in the filename instead of the module name indicates the module was not properly registered in the process.
* 'codechunk' in the filename means that it is a reconstructed dump from a loose executable region. This can be for example injected code that did not have a PE header. Codechunks will be dumped twice, once with a reconstructed x86 and again with a reconstructed x64 header.

Example filenames of dumped files
* notepad_exe_PID2990_hiddenmodule_16B8ABB0000_x86.dll
* notepad_exe_PID3b5c_notepad.exe_7FF6E6630000_x64.exe
* notepad_exe_PID2c54_codechunk_17BD0000_x86.dll
* notepad_exe_PID2c54_codechunk_17BD0000_x64.dll


# Version history

## Version 3.0.0 (unreleased)
* Hardened untrusted PE parsing, reconstruction bounds, output naming/creation, close-monitor cleanup and release artifact handling.
* Improved sparse section packing and content fidelity, PE32/PE64 imports and exports, raw-address reconstruction, and handling of extended/malformed headers. Intentional per-location IAT fixups are preserved.
* Added persistent module-level system scheduling, parallel live/file database hashing and lower-allocation reconstruction, with repeatable output-equivalence benchmarks.
* Improved entrypoint selection: preserve valid executable entrypoints, reject weak-only guesses, and use validated x64 runtime-function metadata as a fallback. Accuracy and remaining false recoveries are documented above.
* Added opt-in `-reexec` preparation and a controlled execution-progress benchmark; ordinary forensic dumps continue to preserve captured state.
* Added native regression tests, portable benchmark/release/CLI checks and Win32/x64 Debug/Release CI with modern Visual Studio toolsets.
* Unified the 3.0.0 banner and Windows version resources, refreshed all CLI flags/defaults/aliases, and made help exit without running a requested dump or database command.

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
