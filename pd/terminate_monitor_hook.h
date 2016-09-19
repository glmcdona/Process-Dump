#pragma once

#include "windows.h"
#include "simple.h"
#include "export_list.h"
#include "utils.h"

class terminate_monitor_hook
{
	HANDLE _ph;
	DWORD _pid;
	bool _is64;
	unsigned __int64 _hook_address;
	unsigned __int64 _address_terminate;
	unsigned __int64 _address_is_waiting;
	unsigned __int64 _address_thread_id;
	unsigned char _original_hook_bytes[32];
	bool _process_is_terminating;
	PD_OPTIONS* _options;

	bool add_redirect(unsigned __int64 target_address);
	
	unsigned __int64 get_address(char* library, char* procedure_name);
	bool get_terminate_orig_code(unsigned char* out_buffer, int length);

public:
	bool hook_terminate(export_list* exports);
	bool unhock_terminate();

	bool is_terminate_waiting();
	void resume_terminate();

	terminate_monitor_hook(HANDLE ph, DWORD pid, bool is64, PD_OPTIONS* _options);
	~terminate_monitor_hook();
};

