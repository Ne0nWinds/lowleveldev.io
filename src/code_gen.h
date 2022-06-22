#pragma once
#include "parser.h"

typedef struct compile_result compile_result;
struct compile_result {
	u32 length;
	u8 *code;
};

compile_result *gen_code(node *ast);
