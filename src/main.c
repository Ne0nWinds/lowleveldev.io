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
	tokenizer_init(src, length);
	u32 function_count = 0;
	func *ast = parse_tokens(&function_count);
	return (ast != 0) ? gen_code(ast, function_count) : 0;
}
