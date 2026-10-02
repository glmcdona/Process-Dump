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
	unsigned __int64 _hook_address = 0;
	unsigned __int64 _address_terminate = 0;
	unsigned char _original_hook_bytes[32] = {};
	bool _original_bytes_valid = false;
	bool _redirect_attempted = false;
	bool _installed = false;
	bool _protection_changed = false;
	DWORD _original_protection = 0;
	HANDLE _release_event = NULL;
	HANDLE _remote_event = NULL;
	PD_OPTIONS* _options;

	bool add_redirect(unsigned __int64 target_address);
	bool restore_protection();
	bool executable_address(unsigned __int64 address);
	bool close_unpublished_event();

public:
	bool hook_terminate(export_list* exports);
	bool unhock_terminate();
	bool is_terminate_waiting();
	bool resume_terminate();

	terminate_monitor_hook(HANDLE ph, DWORD pid, bool is64, PD_OPTIONS* options);
	~terminate_monitor_hook();
	terminate_monitor_hook(const terminate_monitor_hook&) = delete;
	terminate_monitor_hook& operator=(const terminate_monitor_hook&) = delete;
};
