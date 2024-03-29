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
This is designed for Visual Studio 2019 and works with the free Community edition. Just open the project file with VS2019 and compile, it should be that easy!

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
