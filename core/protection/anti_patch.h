#pragma once
#include <cstdint>

namespace anti_patch
{

	inline uint64_t initial_memory_hash = 0;

	void initialize();
	uint64_t get_current_memory_hash();
	void save_memory_hash();
	bool compare_memory_hash();
}

