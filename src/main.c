#include "memory.h"
#include "tokenizer.h"

__attribute__((export_name("compile")))
void *compile(char *src, u32 length) {
	token_list tl = tokenize(src, length);
	return bump_alloc(0);
}
