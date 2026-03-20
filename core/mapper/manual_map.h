#pragma once
#include <Windows.h>
#include <iostream>
#include "pe_parse.h"

class mapper {
public:
    mapper(const wchar_t* dll_path, DWORD target_pid);
    ~mapper();
    bool inject();

private:
    HANDLE m_process;
    BYTE* m_dll_buffer;
    BYTE* m_remote_image;
    DWORD m_pid;

    bool load_dll_to_buffer(const wchar_t* path);
    bool open_target_process();
    bool allocate_remote_memory();
    bool apply_relocations();
    bool resolve_imports();
    bool map_sections();
    bool execute_entry_point();
};