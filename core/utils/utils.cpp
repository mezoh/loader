#include "utils.h"
#include <random>
#include <TlHelp32.h>

void utils::initialize_console()
{
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
	freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
}

std::string utils::generate_random_string(int length)
{
	std::string set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::string random_string;

	std::random_device rd;
	std::mt19937 generator(rd());
	std::uniform_int_distribution<> dist(0, set.length() - 1);

	for (int i = 0; i < length; i++) {
		random_string += set[dist(generator)];
	}
	return random_string;
}

DWORD utils::get_process_id_by_name(std::wstring process_name) {
	DWORD process_id = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	PROCESSENTRY32 process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &process_entry)) {
		do {

			std::wstring exe_file = process_entry.szExeFile;
			if (exe_file == process_name) {
				process_id = process_entry.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &process_entry));
	}

	CloseHandle(snapshot);
	return process_id;
}

uintptr_t utils::get_module_base_address(DWORD process_id, std::wstring module_name)
{
	uintptr_t base_address = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	MODULEENTRY32 module_entry;
	module_entry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &module_entry)) {
		do {

			std::wstring mod_name = module_entry.szModule;
			if (mod_name == module_name) {
				base_address = (uintptr_t)module_entry.modBaseAddr;
				break;
			}
		} while (Module32Next(snapshot, &module_entry));
	}

	CloseHandle(snapshot);
	return base_address;
}

bool utils::read_bytes(HANDLE process_handle, uintptr_t address, void* buffer, SIZE_T size) {
	SIZE_T bytes_read;
	return ReadProcessMemory(process_handle, (LPCVOID)address, buffer, size, &bytes_read) && bytes_read == size;
}
