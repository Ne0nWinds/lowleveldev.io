#include "memory.h"

u8 *alloc_ptr;

// TODO: Page Allocation
void *bump_alloc(u32 size) {
	const u32 alignment = 4;

	size = (size + (alignment - 1)) & ~(alignment - 1);

	void *ptr = alloc_ptr;
	alloc_ptr += size;
	return ptr;
}

__attribute__((export_name("bump_alloc_src_code")))
void *bump_alloc_src_code(u32 size) {
	alloc_ptr = (u8 *)0x4;
	return bump_alloc(size);
}
