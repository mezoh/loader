#pragma once
#include <iostream>
#include <Windows.h>
#include <core/crypt/skCrypter.h>

namespace utils
{
	void initialize_console();
	std::string generate_random_string(int length);
	DWORD get_process_id_by_name(std::wstring process_name);
	uintptr_t get_module_base_address(DWORD process_id, std::wstring module_name);
	bool read_bytes(HANDLE process_handle, uintptr_t address, void* buffer, SIZE_T size);
}

