#include "manual_map.h"
#include <fstream>

mapper::mapper(const wchar_t* dll_path, DWORD target_pid)
    : m_process(nullptr), m_dll_buffer(nullptr), m_remote_image(nullptr), m_pid(target_pid)
{
    load_dll_to_buffer(dll_path);
}

mapper::~mapper()
{
    if (m_dll_buffer)
        delete[] m_dll_buffer;
    if (m_process)
        CloseHandle(m_process);
}

bool mapper::load_dll_to_buffer(const wchar_t* path)
{
    HANDLE file = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << "failed to open dll" << std::endl;
        return false;
    }

    DWORD file_size = GetFileSize(file, nullptr);
    m_dll_buffer = new BYTE[file_size];

    DWORD bytes_read;
    ReadFile(file, m_dll_buffer, file_size, &bytes_read, nullptr);
    CloseHandle(file);

    pe_parse parser(m_dll_buffer);
    if (!parser.is_valid()) {
        std::cerr << "not a valid PE file" << std::endl;
        delete[] m_dll_buffer;
        m_dll_buffer = nullptr;
        return false;
    }

    std::cout << "dll loaded into buffer successfully" << std::endl;
    return true;
}

bool mapper::open_target_process()
{
    m_process = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD,
        FALSE,
        m_pid
    );

    if (!m_process) {
        std::cerr << "failed to open process, error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "opened target process successfully" << std::endl;
    return true;
}

bool mapper::allocate_remote_memory()
{
    pe_parse parser(m_dll_buffer);
    DWORD image_size = parser.get_image_size();

    m_remote_image = reinterpret_cast<BYTE*>(VirtualAllocEx(
        m_process,
        nullptr,
        image_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    ));

    if (!m_remote_image) {
        std::cerr << "failed to allocate remote memory, error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "allocated remote memory successfully" << std::endl;
    std::cout << "allocated: " << std::hex << image_size << " bytes at address: 0x" << m_remote_image << std::dec << std::endl;
    return true;
}

bool mapper::apply_relocations()
{
    pe_parse parser(m_dll_buffer);

    uintptr_t preferred_base = parser.get_nt_headers()->OptionalHeader.ImageBase;
    uintptr_t actual_base = reinterpret_cast<uintptr_t>(m_remote_image);
    uintptr_t delta = actual_base - preferred_base;

    if (delta == 0) {
        std::cout << "no relocations needed, loaded at preferred base" << std::endl;
        return true;
    }

    std::cout << "preferred base: 0x" << std::hex << preferred_base << std::endl;
    std::cout << "actual base:    0x" << actual_base << std::endl;
    std::cout << "delta:          0x" << delta << std::dec << std::endl;

    auto reloc_dir = parser.get_reloc_directory();
    if (!reloc_dir || reloc_dir->VirtualAddress == 0) {
        std::cout << "no reloc directory found" << std::endl;
        return true;
    }

    uintptr_t reloc_offset = parser.rva_to_offset(reloc_dir->VirtualAddress);
    auto block = reinterpret_cast<PIMAGE_BASE_RELOCATION>(m_dll_buffer + reloc_offset);
    auto reloc_end = reinterpret_cast<uintptr_t>(block) + reloc_dir->Size;

    while (reinterpret_cast<uintptr_t>(block) < reloc_end && block->SizeOfBlock) {
        DWORD entry_count = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        WORD* entries = reinterpret_cast<WORD*>(reinterpret_cast<uintptr_t>(block) + sizeof(IMAGE_BASE_RELOCATION));

        for (DWORD i = 0; i < entry_count; i++) {
            int type = entries[i] >> 12;
            int offset = entries[i] & 0xFFF;

            if (type == IMAGE_REL_BASED_DIR64) {
                uintptr_t patch_rva = block->VirtualAddress + offset;
                uintptr_t patch_offset = parser.rva_to_offset(patch_rva);
                uintptr_t* patch = reinterpret_cast<uintptr_t*>(m_dll_buffer + patch_offset);
                *patch += delta;
            }
        }

        block = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
            reinterpret_cast<uintptr_t>(block) + block->SizeOfBlock
            );
    }

    std::cout << "Applied relocations successfully" << std::endl;
    return true;
}

bool mapper::resolve_imports()
{
    pe_parse parser(m_dll_buffer);
    auto import_desc = parser.get_first_import_descriptor();

    if (!import_desc) {
        std::cout << "No imports found" << std::endl;
        return true;
    }

    while (import_desc->Name != 0) {
        uintptr_t name_offset = parser.rva_to_offset(import_desc->Name);
        char* dll_name = reinterpret_cast<char*>(m_dll_buffer + name_offset);

        std::cout << "Resolving imports from: " << dll_name << std::endl;

        HMODULE dll_handle = LoadLibraryA(dll_name);
        if (!dll_handle) {
            std::cerr << "Failed to load dependency: " << dll_name << std::endl;
            return false;
        }

        uintptr_t oft_offset = parser.rva_to_offset(
            import_desc->OriginalFirstThunk ? import_desc->OriginalFirstThunk : import_desc->FirstThunk
        );
        uintptr_t ft_offset = parser.rva_to_offset(import_desc->FirstThunk);

        auto original_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(m_dll_buffer + oft_offset);
        auto first_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(m_dll_buffer + ft_offset);

        while (original_thunk->u1.AddressOfData != 0) {
            uintptr_t func_addr = 0;

            if (IMAGE_SNAP_BY_ORDINAL64(original_thunk->u1.Ordinal)) {
                WORD ordinal = IMAGE_ORDINAL64(original_thunk->u1.Ordinal);
                func_addr = reinterpret_cast<uintptr_t>(
                    GetProcAddress(dll_handle, MAKEINTRESOURCEA(ordinal))
                    );
            }
            else {
                uintptr_t name_offset = parser.rva_to_offset(original_thunk->u1.AddressOfData);
                auto import_by_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(m_dll_buffer + name_offset);
                func_addr = reinterpret_cast<uintptr_t>(
                    GetProcAddress(dll_handle, import_by_name->Name)
                    );
            }

            if (!func_addr) {
                std::cerr << "Failed to resolve import" << std::endl;
                return false;
            }

            first_thunk->u1.Function = func_addr;
            original_thunk++;
            first_thunk++;
        }

        import_desc++;
    }

    std::cout << "Resolved imports successfully" << std::endl;
    return true;
}

bool mapper::map_sections()
{
    pe_parse parser(m_dll_buffer);

    if (!WriteProcessMemory(m_process, m_remote_image, m_dll_buffer, parser.get_headers_size(), nullptr)) {
        std::cerr << "failed to write headers" << std::endl;
        return false;
    }

    auto sections = parser.get_all_sections();
    for (auto section : sections) {
        if (section->SizeOfRawData == 0)
            continue;

        BYTE* source = m_dll_buffer + section->PointerToRawData;
        BYTE* destination = m_remote_image + section->VirtualAddress;

        if (!WriteProcessMemory(m_process, destination, source, section->SizeOfRawData, nullptr)) {
            std::cerr << "failed to map section" << std::endl;
            return false;
        }

        char name[9] = { 0 };
        memcpy(name, section->Name, 8);
        std::cout << "Mapped section: " << name
            << " to 0x" << std::hex << reinterpret_cast<uintptr_t>(destination)
            << " (size: 0x" << section->SizeOfRawData << std::dec << " bytes)" << std::endl;
    }

    return true;
}

bool mapper::execute_entry_point()
{
    pe_parse parser(m_dll_buffer);

    uintptr_t entry_rva = parser.get_nt_headers()->OptionalHeader.AddressOfEntryPoint;
    uintptr_t entry_point = reinterpret_cast<uintptr_t>(m_remote_image) + entry_rva;

    std::cout << "Executing entry point at: 0x" << std::hex << entry_point << std::dec << std::endl;

    HANDLE thread = CreateRemoteThread(
        m_process,
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(entry_point),
        m_remote_image,
        0,
        nullptr
    );

    if (!thread) {
        std::cerr << "failed to create remote thread, error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "remote thread created, waiting for it to finish..." << std::endl;

    WaitForSingleObject(thread, INFINITE);

    DWORD exit_code;
    GetExitCodeThread(thread, &exit_code);
    std::cout << "thread exited with code: " << exit_code << std::endl;

    CloseHandle(thread);
    return true;
}

bool mapper::inject()
{
    if (!open_target_process())
        return false;

    if (!allocate_remote_memory())
        return false;

    if (!apply_relocations())
        return false;

    if (!resolve_imports())
        return false;

    if (!map_sections())
        return false;

    if (!execute_entry_point())
        return false;

    return true;
}