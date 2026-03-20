#include "pe_parse.h"
#include <iomanip>

pe_parse::pe_parse(BYTE* buffer)
{
    this->m_base = buffer;
    m_dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(m_base);
    m_nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(m_base) + m_dos_header->e_lfanew
        );
}

bool pe_parse::is_valid()
{
    if (!m_base)
        return false;

    // Check DOS signature
    if (m_dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Invalid DOS signature" << std::endl;
        return false;
    }

    // Check NT signature
    if (m_nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Invalid NT signature" << std::endl;
        return false;
    }

    return true;
}

bool pe_parse::is_64bit()
{
    if (!is_valid())
        return false;

    return m_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
}

void pe_parse::print_basic_info()
{
    if (!is_valid()) {
        std::cerr << "Invalid PE file" << std::endl;
        return;
    }

    std::cout << "=== PE Basic Information ===" << std::endl;
    std::cout << "Architecture: " << (is_64bit() ? "x64" : "x86") << std::endl;
    std::cout << "Image Base: 0x" << std::hex << get_image_base() << std::dec << std::endl;
    std::cout << "Entry Point: 0x" << std::hex << m_nt_headers->OptionalHeader.AddressOfEntryPoint << std::dec << std::endl;
    std::cout << "Image Size: 0x" << std::hex << get_image_size() << std::dec << " bytes" << std::endl;
    std::cout << "Section Count: " << get_section_count() << std::endl;
    std::cout << "Subsystem: ";

    switch (m_nt_headers->OptionalHeader.Subsystem) {
    case IMAGE_SUBSYSTEM_WINDOWS_GUI: std::cout << "GUI"; break;
    case IMAGE_SUBSYSTEM_WINDOWS_CUI: std::cout << "Console"; break;
    case IMAGE_SUBSYSTEM_NATIVE: std::cout << "Native"; break;
    default: std::cout << "Unknown (" << m_nt_headers->OptionalHeader.Subsystem << ")"; break;
    }
    std::cout << std::endl << std::endl;
}

void pe_parse::print_section_info()
{
    if (!is_valid()) {
        return;
    }

    std::cout << "=== Section Information ===" << std::endl;
    std::cout << std::left << std::setw(10) << "Name"
        << std::setw(12) << "VirtAddr"
        << std::setw(12) << "VirtSize"
        << std::setw(12) << "RawSize"
        << std::setw(12) << "RawPtr"
        << "Characteristics" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    PIMAGE_SECTION_HEADER section_header = IMAGE_FIRST_SECTION(m_nt_headers);

    for (int i = 0; i < m_nt_headers->FileHeader.NumberOfSections; i++) {
        char name[9] = { 0 };
        memcpy(name, section_header[i].Name, 8);

        std::cout << std::left << std::setw(10) << name;
        std::cout << "0x" << std::hex << std::setw(10) << section_header[i].VirtualAddress;
        std::cout << "0x" << std::setw(10) << section_header[i].Misc.VirtualSize;
        std::cout << "0x" << std::setw(10) << section_header[i].SizeOfRawData;
        std::cout << "0x" << std::setw(10) << section_header[i].PointerToRawData;

        // Print characteristics
        DWORD chars = section_header[i].Characteristics;
        if (chars & IMAGE_SCN_MEM_EXECUTE) std::cout << "X";
        if (chars & IMAGE_SCN_MEM_READ) std::cout << "R";
        if (chars & IMAGE_SCN_MEM_WRITE) std::cout << "W";

        std::cout << std::dec << std::endl;
        section_header++;
    }
    std::cout << std::endl;
}

void pe_parse::print_data_directories()
{
    if (!is_valid())
        return;

    std::cout << "=== Data Directories ===" << std::endl;

    const char* dir_names[] = {
        "Export", "Import", "Resource", "Exception",
        "Security", "Base Relocation", "Debug", "Architecture",
        "Global Ptr", "TLS", "Load Config", "Bound Import",
        "IAT", "Delay Import", "COM Descriptor", "Reserved"
    };

    for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
        auto dir = get_data_directory(i);
        if (dir->VirtualAddress != 0) {
            std::cout << std::left << std::setw(20) << dir_names[i]
                << " RVA: 0x" << std::hex << std::setw(10) << dir->VirtualAddress
                << " Size: 0x" << dir->Size << std::dec << std::endl;
        }
    }
    std::cout << std::endl;
}

void pe_parse::print_imports()
{
    if (!is_valid())
        return;

    auto import_desc = get_first_import_descriptor();
    if (!import_desc) {
        std::cout << "No imports found" << std::endl;
        return;
    }

    std::cout << "=== Import Information ===" << std::endl;

    while (import_desc->Name != 0) {
        char* dll_name = reinterpret_cast<char*>(
            reinterpret_cast<uintptr_t>(m_base) + import_desc->Name
            );

        std::cout << "\n" << dll_name << ":" << std::endl;

        // Get the import name table
        PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            reinterpret_cast<uintptr_t>(m_base) +
            (import_desc->OriginalFirstThunk ? import_desc->OriginalFirstThunk : import_desc->FirstThunk)
            );

        int count = 0;
        while (thunk->u1.AddressOfData != 0) {
            if (is_64bit()) {
                if (IMAGE_SNAP_BY_ORDINAL64(thunk->u1.Ordinal)) {
                    std::cout << "  Ordinal: " << IMAGE_ORDINAL64(thunk->u1.Ordinal) << std::endl;
                }
                else {
                    PIMAGE_IMPORT_BY_NAME import_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                        reinterpret_cast<uintptr_t>(m_base) + thunk->u1.AddressOfData
                        );
                    std::cout << "  " << import_name->Name << std::endl;
                }
            }
            else {
                if (IMAGE_SNAP_BY_ORDINAL32(thunk->u1.Ordinal)) {
                    std::cout << "  Ordinal: " << IMAGE_ORDINAL32(thunk->u1.Ordinal) << std::endl;
                }
                else {
                    PIMAGE_IMPORT_BY_NAME import_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                        reinterpret_cast<uintptr_t>(m_base) + thunk->u1.AddressOfData
                        );
                    std::cout << "  " << import_name->Name << std::endl;
                }
            }

            thunk++;
            count++;
            if (count > 100) { // Limit output for very large import tables
                std::cout << "  ... (truncated)" << std::endl;
                break;
            }
        }

        import_desc++;
    }
    std::cout << std::endl;
}

void pe_parse::print_exports()
{
    if (!is_valid())
        return;

    auto export_dir_entry = get_export_directory();
    if (!export_dir_entry || export_dir_entry->VirtualAddress == 0) {
        std::cout << "No exports found" << std::endl;
        return;
    }

    auto export_dir = get_export_directory_data();
    if (!export_dir) {
        std::cout << "Invalid export directory" << std::endl;
        return;
    }

    std::cout << "=== Export Information ===" << std::endl;

    char* dll_name = reinterpret_cast<char*>(
        reinterpret_cast<uintptr_t>(m_base) + export_dir->Name
        );
    std::cout << "Module: " << dll_name << std::endl;
    std::cout << "Number of functions: " << export_dir->NumberOfFunctions << std::endl;
    std::cout << "Number of names: " << export_dir->NumberOfNames << std::endl;

    DWORD* address_of_functions = reinterpret_cast<DWORD*>(
        reinterpret_cast<uintptr_t>(m_base) + export_dir->AddressOfFunctions
        );
    DWORD* address_of_names = reinterpret_cast<DWORD*>(
        reinterpret_cast<uintptr_t>(m_base) + export_dir->AddressOfNames
        );
    WORD* address_of_name_ordinals = reinterpret_cast<WORD*>(
        reinterpret_cast<uintptr_t>(m_base) + export_dir->AddressOfNameOrdinals
        );

    std::cout << "\nExported functions:" << std::endl;
    for (DWORD i = 0; i < export_dir->NumberOfNames; i++) {
        char* func_name = reinterpret_cast<char*>(
            reinterpret_cast<uintptr_t>(m_base) + address_of_names[i]
            );
        WORD ordinal = address_of_name_ordinals[i];
        DWORD func_rva = address_of_functions[ordinal];

        std::cout << "  [" << std::dec << (ordinal + export_dir->Base) << "] "
            << func_name << " (RVA: 0x" << std::hex << func_rva << ")"
            << std::dec << std::endl;

        if (i > 50) { // Limit output
            std::cout << "  ... (truncated)" << std::endl;
            break;
        }
    }
    std::cout << std::endl;
}

// Getters - Addresses
uintptr_t pe_parse::get_entry_point_address()
{
    return reinterpret_cast<uintptr_t>(m_base) + m_nt_headers->OptionalHeader.AddressOfEntryPoint;
}

uintptr_t pe_parse::get_base_of_code()
{
    return reinterpret_cast<uintptr_t>(m_base) + m_nt_headers->OptionalHeader.BaseOfCode;
}

uintptr_t pe_parse::get_image_base()
{
    return m_nt_headers->OptionalHeader.ImageBase;
}

// Getters - Sizes
uint32_t pe_parse::get_image_size()
{
    return m_nt_headers->OptionalHeader.SizeOfImage;
}

uint32_t pe_parse::get_headers_size()
{
    return m_nt_headers->OptionalHeader.SizeOfHeaders;
}

uint32_t pe_parse::get_size_of_code()
{
    return m_nt_headers->OptionalHeader.SizeOfCode;
}

// Getters - Headers
PIMAGE_FILE_HEADER pe_parse::get_file_header()
{
    return &m_nt_headers->FileHeader;
}

PIMAGE_OPTIONAL_HEADER pe_parse::get_optional_header()
{
    return &m_nt_headers->OptionalHeader;
}

// Getters - Sections
PIMAGE_SECTION_HEADER pe_parse::get_section_by_name(const char* section_name)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(m_nt_headers);
    for (int i = 0; i < m_nt_headers->FileHeader.NumberOfSections; i++) {
        if (strncmp((const char*)section->Name, section_name, 8) == 0) {
            return section;
        }
        section++;
    }
    return nullptr;
}

PIMAGE_SECTION_HEADER pe_parse::get_section_by_rva(uintptr_t rva)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(m_nt_headers);
    for (int i = 0; i < m_nt_headers->FileHeader.NumberOfSections; i++) {
        if (rva >= section->VirtualAddress &&
            rva < section->VirtualAddress + section->Misc.VirtualSize) {
            return section;
        }
        section++;
    }
    return nullptr;
}

std::vector<PIMAGE_SECTION_HEADER> pe_parse::get_all_sections()
{
    std::vector<PIMAGE_SECTION_HEADER> sections;
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(m_nt_headers);
    for (int i = 0; i < m_nt_headers->FileHeader.NumberOfSections; i++) {
        sections.push_back(section);
        section++;
    }
    return sections;
}

int pe_parse::get_section_count()
{
    return m_nt_headers->FileHeader.NumberOfSections;
}

// Getters - Data Directories
PIMAGE_DATA_DIRECTORY pe_parse::get_data_directory(int index)
{
    if (index < 0 || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
        return nullptr;

    return &m_nt_headers->OptionalHeader.DataDirectory[index];
}

PIMAGE_DATA_DIRECTORY pe_parse::get_export_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_EXPORT);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_import_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_IMPORT);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_resource_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_RESOURCE);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_exception_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_reloc_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_tls_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_TLS);
}

PIMAGE_DATA_DIRECTORY pe_parse::get_iat_directory()
{
    return get_data_directory(IMAGE_DIRECTORY_ENTRY_IAT);
}

// Import/Export specific
PIMAGE_IMPORT_DESCRIPTOR pe_parse::get_first_import_descriptor()
{
    auto import_dir = get_import_directory();
    if (!import_dir || import_dir->VirtualAddress == 0)
        return nullptr;

    uintptr_t import_offset = rva_to_offset(import_dir->VirtualAddress);

    return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<uintptr_t>(m_base) + import_offset);
}

PIMAGE_EXPORT_DIRECTORY pe_parse::get_export_directory_data()
{
    auto export_dir = get_export_directory();
    if (!export_dir || export_dir->VirtualAddress == 0)
        return nullptr;

    return reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
        reinterpret_cast<uintptr_t>(m_base) + export_dir->VirtualAddress
        );
}

// Utilities
uintptr_t pe_parse::rva_to_offset(uintptr_t rva)
{
    auto section = get_section_by_rva(rva);
    if (!section)
        return 0;

    return rva - section->VirtualAddress + section->PointerToRawData;
}

uintptr_t pe_parse::rva_to_va(uintptr_t rva)
{
    return reinterpret_cast<uintptr_t>(m_base) + rva;
}