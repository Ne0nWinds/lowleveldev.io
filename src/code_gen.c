#include "code_gen.h"

static node **stack;
static u32 stack_size;
static u8 *c;

void gen_expr(node *n);

compile_result *gen_code(node *ast) {

	u8 *code = bump_alloc(0);
	c = code;

	char boilerplate[] = "(module\n\t(func (result i32)\n";
	__builtin_memcpy(c, boilerplate, len(boilerplate) - 1);
	c += len(boilerplate) - 1;

	gen_expr(ast);

	*c++ = '\t';
	*c++ = ')';
	*c++ = '\n';
	*c++ = ')';
	*c++ = '\0';

	bump_alloc(c - code);
	compile_result *result = bump_alloc(sizeof(compile_result));
	result->code = code;
	result->length = c - code;
	return result;
}

void gen_expr(node *n) {

	if (n->type == NODE_INT) {
		char int_code[] = "\t\ti32.const ";
		__builtin_memcpy(c, int_code, len(int_code) - 1);
		c += len(int_code) - 1;
		*c++ = n->value + '0';
		*c++ = '\n';
		return;
	}

	gen_expr(n->left);
	gen_expr(n->right);

	switch (n->type) {
		case NODE_PLUS: {
			char int_code[] = "\t\ti32.add\n";
			__builtin_memcpy(c, int_code, len(int_code) - 1);
			c += len(int_code) - 1;
		} break;
		case NODE_MINUS: {
			char int_code[] = "\t\ti32.sub\n";
			__builtin_memcpy(c, int_code, len(int_code) - 1);
			c += len(int_code) - 1;
		} break;
	}
}
