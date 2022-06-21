#include "memory.h"
#include "tokenizer.h"
#include "parser.h"

__attribute__((export_name("compile")))
void *compile(char *src, u32 length) {
	token_list tl = tokenize(src, length);
	parse_tokens(tl);
	return bump_alloc(0);
}
