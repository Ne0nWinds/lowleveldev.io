#include "parser.h"
#include "memory.h"

token *current_token;

bool error_occurred;

node *expr_stmt();
node *expr();
void expect_token(token_type c);

node *parse_tokens(token_list tokens) {
	error_occurred = false;
	current_token = tokens.tokens;

	return expr_stmt();
}

void expect_token(token_type t) {
	if (current_token->token_type == t) {
		current_token += 1;
	} else {
		error_occurred = true;
	}
}

node *expr_stmt() {
	node *n = expr();
	expect_token(';');
	return n;
}

enum {
	PRECEDENCE_EQUALITY = 1,
	PRECEDENCE_RELATIONAL,
	PRECEDENCE_ADD,
	PRECEDENCE_MUL,
};

u32 get_precedence(node_type type) {
	if (type == NODE_LT || type == NODE_LE || type == NODE_GT || type == NODE_GE) return PRECEDENCE_RELATIONAL;
	if (type == NODE_EQ || type == NODE_NE) return PRECEDENCE_EQUALITY;
	if (type == NODE_MULTIPLY || type == NODE_DIVIDE) return PRECEDENCE_MUL;
	return PRECEDENCE_ADD;
}

node *primary() {
	if (current_token->token_type == '-') {
		current_token += 1;
		node *unary_node = bump_alloc(sizeof(node));
		unary_node->type = NODE_NEGATE;
		unary_node->right = primary();
		return unary_node;
	}

	if (current_token->token_type == '(') {
		current_token += 1;
		node *primary_node = expr();
		expect_token(')');
		return primary_node;
	}

	if (current_token->token_type == TOKEN_INT) {
		node *primary_node = bump_alloc(sizeof(node));
		primary_node->type = NODE_INT;
		primary_node->value = current_token->value;
		current_token += 1;
		return primary_node;
	}
}

node *expr() {
	node *primary_stack = 0;
	node *op_stack = 0;
	node *top_node = 0;

	top_node = primary();
	if (!top_node) return 0;

	for (;;) {
		node_type type = 0;

		if (current_token->token_type == '+') {
			type = NODE_PLUS;
		}

		if (current_token->token_type == '-') {
			type = NODE_MINUS;
		}

		if (current_token->token_type == '*') {
			type = NODE_MULTIPLY;
		}

		if (current_token->token_type == '/') {
			type = NODE_DIVIDE;
		}

		if (current_token->token_type == TOKEN_EQ) {
			type = NODE_EQ;
		}

		if (current_token->token_type == TOKEN_NE) {
			type = NODE_NE;
		}

		if (current_token->token_type == '<') {
			type = NODE_LT;
		}

		if (current_token->token_type == '>') {
			type = NODE_GT;
		}

		if (current_token->token_type == TOKEN_LE) {
			type = NODE_LE;
		}

		if (current_token->token_type == TOKEN_GE) {
			type = NODE_GE;
		}

		if (!type) break;

		u32 prev_prec = (op_stack->type != 0) ? get_precedence(op_stack->type) : 0;
		u32 current_prec = get_precedence(type);

		if (current_prec < prev_prec) {
			node *primary = primary_stack;
			primary_stack = primary_stack->next;

			node *op_node = op_stack;
			op_stack = op_stack->next;

			op_node->right = primary;

			node *local_top = op_node;

			while (op_stack && current_prec < get_precedence(op_stack->type)) {
				primary = primary_stack;
				primary_stack = primary_stack->next;

				op_node->left = primary;

				op_node = op_stack;
				op_stack = op_stack->next;

				op_node->right = local_top;
				local_top = op_node;
			}

			if (!op_stack) {
				local_top->left = top_node;
				top_node = local_top;
			} else {
				local_top->left = primary_stack;
				primary_stack = primary_stack->next;
				local_top->next = primary_stack;
				primary_stack = local_top;
			}
		}

		node *new_node = bump_alloc(sizeof(node));
		new_node->type = type;
		new_node->next = op_stack;
		op_stack = new_node;
		current_token += 1;

		node *primary_node = primary();
		primary_node->next = primary_stack;
		primary_stack = primary_node;
	}

	node *primary = primary_stack;
	primary_stack = primary_stack->next;

	node *op_node = op_stack;
	op_stack = op_stack->next;

	op_node->right = primary;

	node *local_top = op_node;

	while (op_stack) {
		primary = primary_stack;
		primary_stack = primary_stack->next;

		op_node->left = primary;

		op_node = op_stack;
		op_stack = op_stack->next;

		op_node->right = local_top;
		local_top = op_node;
	}

	if (local_top) {
		local_top->left = top_node;
		top_node = local_top;
	}

	return top_node;
}
