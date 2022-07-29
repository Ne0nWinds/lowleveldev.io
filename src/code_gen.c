#include "code_gen.h"
#include "code_gen_wasm.h"

static node **stack;
static u32 stack_size;
static u8 *c;
static bool error_occurred;

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

compile_result *gen_code(func *ast, u32 function_count) {

	u8 *code = bump_alloc(0);
	c = code;
	error_occurred = false;

	c += create_module(c);

	c += create_wasm_layout(c, ast, function_count);

	func *function_stack[function_count];
	function_stack[0] = ast;
	u32 function_stack_length = 1;

	*c++ = SECTION_CODE;
	u8 *code_section_start = c;

	u32 total_size = 0;
	*c++ = function_count;
	for (u32 i = 0; i < function_count; ++i) {
		func *f = function_stack[--function_stack_length];
		if (f->right) function_stack[function_stack_length++] = f->right;
		if (f->left) function_stack[function_stack_length++] = f->left;

		u8 *func_start = c;
		*c++ = 0; // vec(locals)
		gen_code_block(f->body);
		c += end_code_block(c);
		u32 length = c - func_start;
		u32 encoded_integer_length = encode_integer_length(length);
		__builtin_memcpy(func_start + encoded_integer_length, func_start, length);
		c += encode_integer(func_start, length);
	}
	total_size = c - code_section_start;

	u32 encoded_integer_length = encode_integer_length(total_size);
	__builtin_memcpy(code_section_start + encoded_integer_length, code_section_start, total_size);
	c += encode_integer(code_section_start, total_size);

	c += end_module(c);

	bump_alloc(c - code);
	compile_result *result = bump_alloc(sizeof(compile_result));
	result->code = code;
	result->length = c - code;
	return result;
}

void gen_addr(node *n) {
	if (n->type == NODE_VAR) {
		c += i32_const(c, n->var.addr);
		return;
	}
	if (n->type == NODE_DEREF) {
		gen_expr(n->right);
		return;
	}

	error_occurred = true;
}

void gen_expr(node *n) {
	if (error_occurred) return;

	if (n->type == NODE_INT) {
		c += i32_const(c, n->value);
		return;
	}

	if (n->type == NODE_VAR) {
		c += i32_const(c, 0);
		c += i32_load(c, 2, n->var.addr);
		return;
	}

	if (n->type == NODE_FUNC_CALL) {
		c += call(c, n->value);
		return;
	}

	if (n->type == NODE_DEREF) {
		gen_expr(n->right);
		c += i32_load(c, 2, 0);
		return;
	}

	if (n->type == NODE_ADDRESS) {
		c += i32_const(c, n->right->var.addr);
		return;
	}

	if (n->type == NODE_NEGATE) {
		gen_expr(n->right);
		c += i32_const(c, -1);
		c += i32_mul(c);
		return;
	}

	if (n->type == NODE_ASSIGN) {
		gen_addr(n->left);
		gen_expr(n->right);
		c += i32_store(c, 2, 0);
		c += i32_const(c, 0);
		return;
	}

	if (n->type == NODE_INT_DECL) {
		c += i32_const(c, 0);
		gen_expr(n->right);
		c += i32_store(c, 2, n->var.addr);
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
		if (n->loop_stmt.start)
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

	if (n->type == NODE_DO_WHILE) {

		c += loop(c);
		c += block(c);

		gen_code_block(n->loop_stmt.body);
		c += drop(c);

		gen_expr(n->loop_stmt.condition);
		c += i32_eqz(c);
		c += br_if(c, 0);

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
