#include "stdafx.h"
#include "close_watcher.h"


close_watcher::close_watcher(pe_hash_database* clean_db, PD_OPTIONS* options)
{
	_clean_db = clean_db;
	_options = options;
	_monitoring_thread = NULL;
	_monitor_request_stop = false;
}

bool close_watcher::start_monitor()
{
	// Create the main monitoring thread
	if (_monitoring_thread == NULL)
	{
		_monitor_request_stop = false;
		_monitoring_thread = new thread(&close_watcher::_monitor_dump_on_close, this);

		printf("Started monitoring for process closes.\r\n");
	}
	return true;
}

bool close_watcher::stop_monitor()
{
	if (_monitoring_thread != NULL)
	{
		// Cleanly exit the main thread
		_monitor_request_stop = true;
		_monitoring_thread->join();

		delete _monitoring_thread;
		_monitoring_thread = NULL;

		printf("Stopped monitoring for process closes.\r\n");
	}

	return true;
}


void close_watcher::_monitor_dump_on_close()
{
	// List of processes hooked
	unordered_set<DWORD> hooked_pids;
	unordered_map<DWORD, dump_process*> hooked_processes;

	// Create our threads that process the dumping of processes as they close
	thread** threads = new thread*[_options->NumberOfThreads];
	
	for (int i = 0; i < _options->NumberOfThreads; i++)
	{
		threads[i] = new thread(&close_watcher::_dump_process_worker_and_close, this);
	}

	// Hook all processes terminates
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	DWORD myPid = GetCurrentProcessId();

	while (!_monitor_request_stop)
	{
		// Keep hooking any new processes
		snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snapshot != INVALID_HANDLE_VALUE)
		{
			if (Process32First(snapshot, &entry) == TRUE)
			{
				while (Process32Next(snapshot, &entry) == TRUE)
				{
					if ( myPid != entry.th32ProcessID && hooked_pids.count(entry.th32ProcessID) == 0)
					{
						if (_wcsicmp(entry.szExeFile, L"csrss.exe") != 0) // TEMPORARY FIX TO ISSUE #10 CRASHING CSRSS.EXE
						{
							// Test code to only hook notepad.exe
							//if (_wcsicmp(entry.szExeFile, L"notepad.exe") == 0)
							//{
								// New process
								dump_process* dumper = new dump_process(entry.th32ProcessID, _clean_db, _options, true);
								if (dumper->monitor_close_start())
								{
									printf("...hooked close of: pid 0x%x,%S\r\n", entry.th32ProcessID, entry.szExeFile);
									hooked_processes.insert(std::pair<DWORD, dump_process*>(dumper->get_pid(), dumper));
									hooked_pids.insert(dumper->get_pid());
								}
								else
									delete dumper;
							//}
						}
					}
				}
			}
			CloseHandle(snapshot);
		}

		// Check if any processes are waiting to close
		for (unordered_map<DWORD, dump_process*>::iterator it = hooked_processes.begin(); it != hooked_processes.end(); )
		{
			if (it->second->monitor_close_is_waiting())
			{

				// Dump this process by adding it to the multi-threaded dumping queue
				char name[0x200];
				it->second->get_process_name(name, sizeof(name));
				printf("Process %s requesting to close, we are dumping it...\r\n", name);
				_work_queue.push(it->second); // Will be freed when it gets processed from work queue

				// Remove this process
				it = hooked_processes.erase(it);
			}
			else
			{
				it++;
			}
		}

		Sleep(10);
	}

	// Wait for the work queue to finish processing
	while (!_work_queue.empty())
	{
		printf("waiting for dump commands to be pulled from work queue...\r\n");
		Sleep(200);
	}

	// Wait for all worker threads to complete
	for (int i = 0; i < _options->NumberOfThreads; i++)
	{
		threads[i]->join(); // blocks until each thread is finished
		delete threads[i];
		threads[i] = NULL;
	}
	delete[]threads;

	// Clean up list of processes hooked
	for (unordered_map<DWORD, dump_process*>::iterator it = hooked_processes.begin(); it != hooked_processes.end(); ++it)
	{
		delete it->second; // cleans up the hook in the destructor
	}
}


void close_watcher::_dump_process_worker_and_close()
{
	// Dump this process
	unordered_set<unsigned __int64> new_hashes;
	while (!_monitor_request_stop || !_work_queue.empty())
	{
		// Process the hashes for this process
		dump_process* entry;
		if (_work_queue.pop(entry))
		{
			// Process this process

			// Dump this process
			entry->monitor_close_dump_and_resume();

			// We're done with the process
			delete entry;
		}

		Sleep(10);
	}
}


close_watcher::~close_watcher()
{
	stop_monitor();
}
