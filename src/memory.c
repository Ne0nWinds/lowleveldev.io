#include "memory.h"

static const u32 page_size = 65536;
static void *alloc_ptr = page_size * 2;

// TODO: Page Allocation
void *bump_alloc(u32 size) {
	const u32 alignment = 4;

	size += (4 - (size % 4));

	void *ptr = (void *)alloc_ptr;
	alloc_ptr += size;
	return ptr;
}

__attribute__((export_name("bump_alloc_src_code")))
void *bump_alloc_src_code(u32 size) {
	void *start = page_size * 2;
	u32 bytes_to_zero = (u8 *)alloc_ptr - (u8 *)start;

	__builtin_memset(start, 0, bytes_to_zero);

	if (__builtin_wasm_memory_size(0) == 2) {
		__builtin_wasm_memory_grow(0, 1);
	}

	alloc_ptr = start;
	return bump_alloc(size);
}
