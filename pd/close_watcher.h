#pragma once
#include "work_queue.h"
#include <thread>
#include "pe_hash_database.h"
#include "dump_process.h"
#include "windows.h"

class close_watcher
{
	pe_hash_database* _clean_db;
	PD_OPTIONS* _options;

	Queue<dump_process*> _work_queue;

	thread* _monitoring_thread;
	bool _monitor_request_stop;

	void _monitor_dump_on_close();
	void _dump_process_worker_and_close();

public:
	close_watcher(pe_hash_database* clean_db, PD_OPTIONS* options);
	bool start_monitor();
	bool stop_monitor();
	~close_watcher();
};

