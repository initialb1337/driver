#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "driver.hpp"

static DWORD GetProcessId(const wchar_t* process_name) {
	DWORD process_id = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}
	PROCESSENTRY32W process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(snapshot, &process_entry)) {
		// Check if the first handle is the one we want.
		if (_wcsicmp(process_entry.szExeFile, process_name) == 0) {
			process_id = process_entry.th32ProcessID;
		}
		else {
			while (Process32NextW(snapshot, &process_entry)) {
				if (_wcsicmp(process_entry.szExeFile, process_name) == 0) {
					process_id = process_entry.th32ProcessID;
					break;
				}
			}
		}
	}

	CloseHandle(snapshot);
	return process_id;
}

static std::uintptr_t GetModuleBase(const DWORD pid, const wchar_t* module_name) {
	std::uintptr_t module_base = 0;

	// Snapshot of process modules (dlls).
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	MODULEENTRY32W module_entry;
	module_entry.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(snapshot, &module_entry)) {
		if (wcsstr(module_name, module_entry.szModule) != nullptr) {
			module_base = reinterpret_cast<std::uintptr_t>(module_entry.modBaseAddr);
		}
		else {
			while (Module32NextW(snapshot, &module_entry)) {
				if (wcsstr(module_name, module_entry.szModule) != nullptr) {
					module_base = reinterpret_cast<std::uintptr_t>(module_entry.modBaseAddr);
					break;
				}
			}
		}
	}

	CloseHandle(snapshot);
	return module_base;
}

int main() {
	const DWORD pid = GetProcessId(L"notepad.exe");
	if (pid == 0) {
		std::cout << "Failed to find process (notepad.exe).\n";
		std::cin.get();
		return 1;
	}

	auto driver = new driver_manager("\\\\.\\baredriver", pid); //create a driver handle
	if (driver == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		std::cout << "Failed to create our driver handle. Error: " << error << "\n";
		std::cin.get();
		return 1;
	}

	CloseHandle(driver);

	std::cin.get();

	return 0;
}