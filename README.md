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
  http://split-code.com/processdump.html

# Installation
You can download the latest compiled release of Process Dump here:
* http://www.split-code.com/files/pd_v2_1.zip

This tool requires Microsoft Visual C++ Redistributable for Visual Studio 2015 to be installed to work:
* https://www.microsoft.com/en-ca/download/details.aspx?id=48145

# Compiling source code
This is designed for Visual Studio 2015 and works with the free Community edition. Just open the project file with VS2015 and compile, it should be that easy!


# Example Usage
Dump all modules and hidden code chunks from all processes on your system (ignoring known clean modules):
* pd64.exe -system

Run in terminate monitor mode. Until cancelled (CTRL-C), Process Dump will dump any process just before the termination:
* pd64.exe -closemon

Dump all modules and hidden code chunks from a specific process identifier:
* pd64.exe -pid 0x18A

Dump all modules and hidden code chunk by process name:
* pd64.exe -p .\*chrome.\*

Build clean-hash database. These hashes will be used to exclude modules from dumping with the above commands:
* pd64.exe -db gen

Dump code from a specific address in PID 0x1a3:
* pd64.exe -pid 0x1a3 -a 0xffb4000
 * Generates two files (32 and 64 bit) that can be loaded for analysis in IDA with generated PE headers and generated import table:
  * notepad_exe_x64_hidden_FFB40000.exe
  * notepad_exe_x86_hidden_FFB40000.exe


# Example sandbox usage
If you are running an automated sandbox or manual anti-malware research environment, I recommend running the following process with Process Dump, run all commands as Administrator:
* On your clean environment build the clean hash database:
 * pd64.exe -db gen
 * (or more quickly) pd64 -db genquick
* Begin the Process Dump terminate monitor. Leave this running in the background to dump all the intermediate processes used by the malware:
 * pd64.exe -closemon
* Run the malware file
* Watch the malware install (and pd64 dumping any process that tries to close)
* When you are ready to dump the running malware from memory, run the following command to dump all processes:
 * pd64.exe -system
* All the dumped components will be in the working directory of pd64.exe. You can change the output path using the '-o' flag,


Notes on the naming convention of dumped modules:
* 'hiddemodule' in the filename instead of the module name indicates the module was not properly registered in the process.
* 'codechunk' in the filename means that it is a reconstructed dump from a loose executable region. This can be for example injected code that did not have a PE header. Codechunks will be dumped twice, once with a reconstructed x86 and again with a reconstructed x64 header.

Example filenames of dumped files
* notepad_exe_PID2990_hiddenmodule_16B8ABB0000_x86.dll
* notepad_exe_PID3b5c_notepad.exe_7FF6E6630000_x64.exe
* notepad_exe_PID2c54_codechunk_17BD0000_x86.dll
* notepad_exe_PID2c54_codechunk_17BD0000_x64.dll


# Help Page
Process Dump v2.1
  Copyright ® 2017, Geoff McDonald
  http://www.split-code.com/

Process Dump (pd.exe) is a tool used to dump both 32 and 64 bit executable modules back to disk from memory within a process address space. This tool is able to find and dump hidden modules as well as loose executable code chunks, and it uses a clean hash database to exclude dumping of known clean files. This tool uses an aggressive import reconstruction approach that links all DWORD/QWORDs that point to an export in the process to the corresponding export function. Process dump can be used to dump all unknown code from memory ('-system' flag), dump specific processes, or run in a monitoring mode that dumps all processes just before they terminate.

Before first usage of this tool, when on the clean workstation the clean exclusing hash database can be generated by either:
* pd -db gen
* pd -db genquick

Example Usage:
* pd -system
* pd -pid 419
* pd -pid 0x1a3
* pd -pid 0x1a3 -a 0x401000 -o c:\dump\ -c c:\dump\test\clean.db
* pd -p chrome.exe
* pd -p "(?i).\*chrome.\*"
* pd -closemon

Options:

* -system
> Dumps all modules not matching the clean hash databas from all accessible processes into the working directory.

* -pid \<pid\>
> Dumps all modules not matching the clean hash database from the specified pid into the current working directory. Use a '0x' prefix to specify a hex PID.

* -closemon
> Runs in monitor mode. When any processes are terminating process dump will first dump the process.

* -p \<regex\>
> Dumps all modules not matching the clean hash database from the process name found to match the filter into specified pid into the current working directory.

* -a \<module base address\>
> Dumps a module at the specified base address from the process.

* -g
> Forces generation of PE headers from scratch, ignoring existing headers.

* -o \<path\>
> Sets the default output root folder for dumped components.

* -v
> Verbose.

* -nh
> No header is printed in the output.

* -nr
> Disable recursion on hash database directory add or remove commands.

* -ni
> Disable import reconstruction.

* -nc
> Disable dumping of loose code regions.

* -nt
> Disable multithreading.

* -t \<thread count\>
> Sets the number of threads to use (default 16).

* -c \<filepath\>
> Full filepath to the clean hash database to use for this run.

* -db gen
> Automatically processes a few common folders as well as
all the currently running processes and adds the found module hashes to the clean hash database. It will add all files recursively in:
  %WINDIR%
  %HOMEPATH%
  C:\Program Files\
  C:\Program Files (x86)\
As well as all modules in all running processes

* -db genquick
> Adds the hashes from all modules in all processes to the clean hash database. Run this on a clean system.

* -db add \<dir\>
> Adds all the files in the specified directory recursively to the clean hash database.

* -db rem \<dir\>
> Removes all the files in the specified directory recursively from the clean hash database.

* -db clean
> Clears the clean hash database.

* -db ignore
> Ignores the clean hash database when dumping a process this time.  All modules will be dumped even if a match is found.



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
   malware from memory beofre the malicious process closes.
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
