# Process Dump
Process Dump is a Windows reverse-engineering command-line tool to dump malware memory components back to disk for analysis. Often malware files are packed and obfuscated before they are executed in order to avoid AV scanners, however when these files are executed they will often unpack or inject a clean version of the malware code in memory. A common task for malware researchers when analyzing malware is to dump this unpacked code back from memory to disk for scanning with AV products or for analysis with static analysis tools such as IDA.

Process Dump works for Windows 32 and 64 bit operating systems and can dump memory components from specific processes or from all processes currently running. Process Dump supports creation and use of a clean-hash database, so that dumping of known files such as kernel32.dll can be skipped. Its main features include:
* Dumps code from a specific process or all processes.
* Finds and dumps hidden modules that are not properly loaded in processes.
* Finds and dumps loose code chunks even if they aren't associated with a PE file. It builds a PE header and import table for the chunks.
* Reconstructs imports using an aggressive approach.
* Can run in close dump monitor mode (`-closemon`), where hooked processes will be paused and dumped just before they terminate.
* Multi-threaded, so when you are dumping all running processes it will go pretty quickly.
* Can generate a clean hash database. Generate this on a clean system to skip known modules during later dumps.

I'm maintaining an official compiled release on my website here:
  https://split-code.com/processdump.html

# Installation
You can download the latest compiled release of Process Dump here:
* https://github.com/glmcdona/Process-Dump/releases

Use `pd64.exe` on 64-bit Windows (recommended), or `pd32.exe` on 32-bit Windows. Both binaries are standalone; no Microsoft Visual C++ Redistributable is required.

# Command-line arguments
Process Dump can dump all unknown code from memory (`-system`), dump specific processes, or monitor processes and dump them before they terminate.

On a clean workstation, generate a baseline database with either:
* `pd64.exe -db genquick`
* `pd64.exe -db gen`

Run database generation and dumping as separate commands. For best access, run as Administrator. The default worker count is 16.

Example usage:
* `pd64.exe -system`
* `pd64.exe -pid 419`
* `pd64.exe -pid 0x1a3`
* `pd64.exe -pid 0x1a3 -a 0x401000 -o C:\dumps`
* `pd64.exe -p "chrome[.]exe"`
* `pd64.exe -p ".*chrome.*"`
* `pd64.exe -closemon`

**General Dumping Options**

| Option | Description |
|--------|-------------|
| `-system` | Dumps modules and loose code not matching the clean hash database from all accessible processes. |
| `-pid <pid>` | Dumps a specific process. Use decimal or a `0x` prefix for a hexadecimal PID. |
| `-closemon` | Hooks process termination and dumps before exit. Use in a controlled environment; press Ctrl+C to stop. |
| `-p <regex>` | Dumps processes whose entire name matches a case-sensitive regular expression. Multiple matches prompt for confirmation. |
| `-a <address>` | Dumps at the specified base address. Requires `-pid`; accepts decimal or `0x`-prefixed hex. |
| `-o <path>` | Sets the output folder (default: current working directory). Create the folder first; existing files are never overwritten. |

**Clean Hash Database Options**

Database files default to the application directory: `clean.hashes`, `entrypoints.hashes` and `shortentrypoints.hashes`.

| Option | Description |
|--------|-------------|
| `-db gen` | Adds hashes from running processes and PE files recursively in `%WINDIR%`, `%USERPROFILE%`, `C:\Program Files` and `C:\Program Files (x86)`. |
| `-db genquick` | Adds hashes from running processes only. Faster than `gen`, but less complete. |
| `-db add <dir>` | Adds PE file hashes from the specified directory recursively, including entrypoint signatures. |
| `-db remove <dir>`, `-db rem <dir>` | Removes matching clean-module hashes from the directory recursively. Entrypoint signatures are retained. |
| `-nr` | Disables recursion for database add/remove commands. Does not affect `-db gen`. |
| `-db clean` | Clears all three databases, including entrypoint signatures. |
| `-db ignore` | Ignores all three loaded databases for this dump without changing the files. Repeated modules may still be skipped within the run. |
| `-cdb <filepath>`, `-c <filepath>` | Sets the clean hash database path. |
| `-edb <filepath>` | Sets the entrypoint hash database path. |
| `-esdb <filepath>` | Sets the short entrypoint hash database path. |

**Output Options**

| Option | Description |
|--------|-------------|
| `-v` | Prints verbose diagnostics. |
| `-nh` | Suppresses the version/copyright banner. |
| `--help`, `-help`, `-h`, `--h` | Prints help and exits. Running without arguments also prints help. |

**Advanced Options**

| Option | Description |
|--------|-------------|
| `-g` | Forces generation of both 32-bit and 64-bit PE headers, ignoring existing headers. |
| `-eprec` | Forces entrypoint recovery even if a valid entrypoint exists. Recovery is heuristic; an unsuccessful search leaves the old value unchanged. |
| `-ni` | Disables aggressive import reconstruction (enabled by default). |
| `-nc` | Disables loose-code dumping and hashing (enabled by default). |
| `-nep` | Disables entrypoint recovery and live entrypoint hashing, overriding `-eprec`. File database generation still collects entrypoint signatures. |
| `-reexec` | Experimental preparation for re-running dumps. Discards some captured state and assumes the original section layout; does not run the dump or guarantee it will work. Off by default. |
| `-nt` | Uses one worker, equivalent to `-t 1`. Close monitoring still has a separate monitoring thread. |
| `-t <count>` | Sets the worker count (default 16, minimum 1) for system dumping, database generation and close monitoring. Direct PID/name dumps remain synchronous. The last `-t`/`-nt` wins. |

# Usage Examples

| Command | Description |
| ------- | ----------- |
| `pd64.exe -db genquick` | Quickly builds a baseline from currently running processes. |
| `pd64.exe -system -t 4 -o C:\dumps` | Dumps unknown modules and loose code with four workers into an existing output folder. |
| `pd64.exe -closemon` | Runs in terminate monitor mode. |
| `pd64.exe -pid 0x18A` | Dumps modules and loose code from a specific process. |
| `pd64.exe -p ".*chrome.*"` | Dumps processes with names containing `chrome`. |
| `pd64.exe -db gen` | Builds a more complete baseline from running processes and common folders. |
| `pd64.exe -db add C:\baseline -nr` | Adds PE files from a directory without searching subdirectories. |
| `pd64.exe -pid 0x1a3 -a 0x401000` | Dumps at a specific address. If no PE header exists, generates both 32-bit and 64-bit headers. |

## Sandbox Usage

For malware analysis, use an isolated lab and run Process Dump as Administrator:

* Build the clean hash database with `pd64.exe -db gen` or `pd64.exe -db genquick` before introducing the sample.
* Keep `pd64.exe -closemon` running to capture hooked processes as they terminate.
* Use `pd64.exe -system` to dump processes that are still running.
* Press Ctrl+C to stop the monitor.

Dumps are written to the working directory unless `-o` is set. Keep them private. Dumps are for analysis and are not guaranteed to run; images larger than 256 MiB are rejected.

# Notes on the naming convention of dumped modules:
* `hiddenmodule` instead of a module name indicates the module was not properly registered in the process.
* `codechunk` means a dump from a loose executable region without a PE header. These are dumped twice, with reconstructed x86 and x64 headers.

Example filenames:
* notepad_exe_PID2990_hiddenmodule_16B8ABB0000_x86.dll
* notepad_exe_PID3b5c_notepad.exe_7FF6E6630000_x64.exe
* notepad_exe_PID2c54_codechunk_17BD0000_x86.exe
* notepad_exe_PID2c54_codechunk_17BD0000_x64.exe

# Version history

## Version 3.0.0 (October 2nd, 2026)
* Fixed oversized and empty dumps, and improved section and import reconstruction.
* Improved entrypoint recovery.
* Sped up system dumping and database generation.
* Fixed security and reliability issues, and added automated tests.

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

# Compiling source code
Open `pd.sln` in Visual Studio 2019 or 2022 (Community edition works) with the Desktop development with C++ workload and a Windows SDK installed. Build Release for Win32 or x64; Release builds statically link the runtime.

To build and run the tests from a Visual Studio Developer PowerShell:

```powershell
msbuild pd.sln /m /p:Configuration=Release /p:Platform=x64
.\x64\Release\pd_tests.exe
```

Use `Platform=Win32` for a 32-bit build. Python tests can be run with `python -m unittest discover -s tests -p "test_*.py"`.
