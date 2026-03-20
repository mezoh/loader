#include "anti_patch.h"
#include <Windows.h>
#include <core/crypt/skCrypter.h>

void anti_patch::initialize()
{
	save_memory_hash();
}

// Linker provided base address
extern "C" IMAGE_DOS_HEADER __ImageBase;

uint64_t anti_patch::get_current_memory_hash()
{
	uint8_t* base = reinterpret_cast<uint8_t*>(&__ImageBase);

	auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		return 0;
	}

	auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) {
		return 0;
	}

	uint64_t combined_hash = 0;

	auto section_header = IMAGE_FIRST_SECTION(nt);
	for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, section_header++) {
		if (section_header->Characteristics & IMAGE_SCN_MEM_WRITE) {
			continue;
		}

		uint8_t* section_start = base + section_header->VirtualAddress;
		size_t section_size = section_header->Misc.VirtualSize;

		uint64_t hash = 0xDEADBEEF;

		for (size_t j = 0; j < section_size; j++) {
			hash ^= section_start[j];
			hash = (hash << 3) | (hash >> (64 - 3));
			hash *= 0x1337;
		}
		combined_hash ^= hash;

	}
	return combined_hash;
}

void anti_patch::save_memory_hash()
{
	initial_memory_hash = get_current_memory_hash();
}

bool anti_patch::compare_memory_hash()
{
	uint64_t current_memory_hash = get_current_memory_hash();

	if (current_memory_hash == 0 || current_memory_hash != initial_memory_hash) {
		return false;
	}
	return true;
}