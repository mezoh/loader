#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

class pe_parse {
public:
    pe_parse(BYTE* buffer);

    // Validation
    bool is_valid();
    bool is_64bit();

    // Basic info
    void print_section_info();
    void print_data_directories();
    void print_imports();
    void print_exports();
    void print_basic_info();

    // Getters - Addresses
    uintptr_t get_entry_point_address();
    uintptr_t get_base_of_code();
    uintptr_t get_image_base();

    // Getters - Sizes
    uint32_t get_image_size();
    uint32_t get_headers_size();
    uint32_t get_size_of_code();

    // Getters - Headers
    PIMAGE_DOS_HEADER get_dos_header() { return m_dos_header; }
    PIMAGE_NT_HEADERS get_nt_headers() { return m_nt_headers; }
    PIMAGE_FILE_HEADER get_file_header();
    PIMAGE_OPTIONAL_HEADER get_optional_header();

    // Getters - Sections
    PIMAGE_SECTION_HEADER get_section_by_name(const char* section_name);
    PIMAGE_SECTION_HEADER get_section_by_rva(uintptr_t rva);
    std::vector<PIMAGE_SECTION_HEADER> get_all_sections();
    int get_section_count();

    // Getters - Data Directories
    PIMAGE_DATA_DIRECTORY get_data_directory(int index);
    PIMAGE_DATA_DIRECTORY get_export_directory();
    PIMAGE_DATA_DIRECTORY get_import_directory();
    PIMAGE_DATA_DIRECTORY get_resource_directory();
    PIMAGE_DATA_DIRECTORY get_exception_directory();
    PIMAGE_DATA_DIRECTORY get_reloc_directory();
    PIMAGE_DATA_DIRECTORY get_tls_directory();
    PIMAGE_DATA_DIRECTORY get_iat_directory();

    // Import related
    PIMAGE_IMPORT_DESCRIPTOR get_first_import_descriptor();

    // Export related
    PIMAGE_EXPORT_DIRECTORY get_export_directory_data();

    // Utilities
    uintptr_t rva_to_offset(uintptr_t rva);
    uintptr_t rva_to_va(uintptr_t rva);

private:
    BYTE* m_base;
    PIMAGE_DOS_HEADER m_dos_header;
    PIMAGE_NT_HEADERS m_nt_headers;
};