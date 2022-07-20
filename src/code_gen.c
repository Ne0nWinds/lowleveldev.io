#include "code_gen.h"
#include "code_gen_wasm.h"

static node **stack;
static u32 stack_size;
static u8 *c;

void gen_expr(node *n);
void gen_code_block(node *n) {
	node *current_expr = n;
	while (current_expr->next) {
		gen_expr(current_expr);
		c += drop(c);
		current_expr = current_expr->next;
	}
	gen_expr(current_expr);
}

compile_result *gen_code(node *ast) {

	u8 *code = bump_alloc(0);
	c = code;

	c += create_module(c);

	c += create_main_function(c);

	u8 *code_section_start = c;

	gen_code_block(ast);

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
		c += i32_const(c, n->value);
		return;
	}

	if (n->type == NODE_VAR) {
		c += i32_const(c, 0);
		c += i32_load(c, 2, n->addr);
		return;
	}

	if (n->type == NODE_NEGATE) {
		gen_expr(n->right);
		c += i32_const(c, -1);
		c += i32_mul(c);
		return;
	}

	if (n->type == NODE_ASSIGN) {
		c += i32_const(c, 0);
		gen_expr(n->right);
		c += i32_store(c, 2, n->left->addr); // hardcoded to be a variable
		c += i32_const(c, 0);
		return;
	}

	if (n->type == NODE_INT_DECL) {
		c += i32_const(c, 0);
		gen_expr(n->right);
		c += i32_store(c, 2, n->addr);
		c += i32_const(c, 0);
		return;
	}

	if (n->type == NODE_IF) {
		gen_expr(n->if_stmt.cond);
		c += wasm_if(c);

		if (n->if_stmt.body) {
			gen_code_block(n->if_stmt.body);
			c += drop(c);
		}

		if (n->if_stmt.else_stmt) {
			c += wasm_else(c);
			gen_code_block(n->if_stmt.else_stmt);
			c += drop(c);
		}
		c += end_code_block(c);

		c += i32_const(c, 0);
		return;
	}

	if (n->type == NODE_LOOP) {
		gen_expr(n->loop_stmt.start);

		c += loop(c);
		c += block(c);

		gen_expr(n->loop_stmt.condition);
		c += i32_eqz(c);
		c += br_if(c, 0);

		gen_code_block(n->loop_stmt.body);
		c += drop(c);

		if (n->loop_stmt.iteration) {
			gen_code_block(n->loop_stmt.iteration);
			c += drop(c);
		}

		c += br(c, 1);
		c += end_code_block(c);
		c += end_code_block(c);
		c += i32_const(c, 0);
		return;
	}

	if (n->type == NODE_RETURN) {
		gen_expr(n->right);
		c += wasm_return(c);
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
		} break;
		case NODE_EQ: {
			c += i32_eq(c);
		} break;
		case NODE_NE: {
			c += i32_ne(c);
		} break;
		case NODE_GT: {
			c += i32_gt_s(c);
		} break;
		case NODE_LT: {
			c += i32_lt_s(c);
		} break;
		case NODE_GE: {
			c += i32_ge_s(c);
		} break;
		case NODE_LE: {
			c += i32_le_s(c);
		} break;
	}
}
