#include "memory.h"
#include "tokenizer.h"
#include "parser.h"
#include "code_gen.h"

enum code_output {
	OUTPUT_WASM = 1,
	OUTPUT_WAT
};

__attribute__((export_name("compile")))
compile_result *compile(char *src, u32 length, enum code_output out) {
	token_list tl = tokenize(src, length);
	node *ast = parse_tokens(tl);
	return gen_code(ast);
}
