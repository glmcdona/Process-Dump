#pragma once

#include <windows.h>
#include <stdio.h>
#include <string>
#include <string.h>

namespace dump_path
{
	// Target-provided labels must never introduce another path component.
	inline bool sanitize_label(const char* label, std::string& result)
	{
		result.clear();
		if (label == NULL || *label == 0)
			label = "unknown";
		for (const unsigned char* ch = reinterpret_cast<const unsigned char*>(label); *ch != 0; ++ch)
		{
			if (result.size() == 255)
				return false;
			const bool allowed = (*ch >= 'a' && *ch <= 'z') || (*ch >= 'A' && *ch <= 'Z') ||
				(*ch >= '0' && *ch <= '9') || *ch == '.' || *ch == '_' || *ch == '-';
			result.push_back(allowed ? static_cast<char>(*ch) : '_');
		}
		return true;
	}

	inline bool is_device_component(const std::string& component)
	{
		std::string stem = component.substr(0, component.find('.'));
		for (char& ch : stem)
			if (ch >= 'a' && ch <= 'z')
				ch = static_cast<char>(ch - 'a' + 'A');
		return stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
			(stem.size() == 4 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0) &&
				stem[3] >= '1' && stem[3] <= '9');
	}

	inline bool make_filename(const char* root, const char* process_name, DWORD pid, const char* module_name,
		unsigned __int64 base, bool is64, const char* extension, std::string& result, std::string& error)
	{
		result.clear();
		error.clear();
		std::string process_label, module_label;
		if (!sanitize_label(process_name, process_label) || !sanitize_label(module_name, module_label))
		{
			error = "a dump label exceeds the Windows 255-byte component limit";
			return false;
		}
		if (extension == NULL || (strcmp(extension, "exe") != 0 && strcmp(extension, "dll") != 0 &&
			strcmp(extension, "sys") != 0 && strcmp(extension, "bin") != 0))
		{
			error = "unsupported dump extension";
			return false;
		}
		char pid_part[32], suffix[48];
		sprintf_s(pid_part, "_PID%x_", pid);
		sprintf_s(suffix, "_%llX_%s.%s", base, is64 ? "x64" : "x86", extension);
		std::string component = process_label + pid_part + module_label + suffix;
		if (component.size() > 255 || is_device_component(component))
		{
			error = "dump filename exceeds the Windows component limit or names a reserved device";
			return false;
		}
		std::string candidate = root == NULL ? "" : root;
		if (!candidate.empty() && candidate.back() != '\\' && candidate.back() != '/')
			candidate += '\\';
		candidate += component;
		if (candidate.size() >= MAX_PATH)
		{
			error = "dump path exceeds the supported Windows MAX_PATH limit";
			return false;
		}
		result.swap(candidate);
		return true;
	}
}
