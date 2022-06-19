#include "memory.h"

__attribute__((export_name("compile")))
void *compile(char *src, u32 length) {
	return bump_alloc(0);
}
