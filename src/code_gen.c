#include "code_gen.h"
#include "code_gen_wasm.h"

static node **stack;
static u32 stack_size;
static u8 *c;

void gen_expr(node *n);

compile_result *gen_code(node *ast) {

	u8 *code = bump_alloc(0);
	c = code;

	c += create_module(c);

	c += create_main_function(c);

	u8 *code_section_start = c;
	gen_expr(ast);
	c += end_code_block(c);

	c += create_code_section(code_section_start, c - code_section_start);

	c += end_module(c);

	bump_alloc(c - code);
	compile_result *result = bump_alloc(sizeof(compile_result));
	result->code = code;
	result->length = c - code;
	return result;
}

void gen_expr(node *n) {
	if (!n) return;

	if (n->type == NODE_INT) {
		// char int_code[] = "\t\ti32.const ";
		// __builtin_memcpy(c, int_code, len(int_code) - 1);
		// c += len(int_code) - 1;
		// *c++ = n->value + '0';
		// *c++ = '\n';
		c += i32_const(c, n->value);
		return;
	}

	gen_expr(n->left);
	gen_expr(n->right);

	switch (n->type) {
		case NODE_PLUS: {
			c += i32_add(c);
		} break;
		case NODE_MINUS: {
			c += i32_sub(c);
		} break;
		case NODE_MULTIPLY: {
			c += i32_mul(c);
		} break;
		case NODE_DIVIDE: {
			c += i32_div_s(c);
			// char int_code[] = "\t\ti32.div_u\n";
			// __builtin_memcpy(c, int_code, len(int_code) - 1);
			// c += len(int_code) - 1;
		} break;
	}
}
