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
	if (current_token->token_type != t) {
		error_occurred = true;
		current_token += 1;
	}
}

node *expr_stmt() {
	node *n = expr();
	expect_token(';');
	return n;
}

enum {
	PRECEDENCE_ADD = 1,
	PRECEDENCE_MUL,
};

u32 get_precedence(node_type type) {
	if (type == NODE_MULTIPLY) return PRECEDENCE_MUL;
	return PRECEDENCE_ADD;
}

node *expr() {
	node *primary_stack = bump_alloc(sizeof(node));
	primary_stack->type = 0;
	node *op_stack = bump_alloc(sizeof(node));
	op_stack->type = 0;

	node *top_node = 0;

	for (;;) {
		if (current_token->token_type == TOKEN_INT) {
			node *primary_node = bump_alloc(sizeof(node));
			primary_node->type = NODE_INT;
			primary_node->value = current_token->value;
			primary_node->left = primary_stack;
			primary_stack = primary_node;
			current_token += 1;
		} else break;

		node_type type = 0;

		if (current_token->token_type == '+') {
			type = NODE_PLUS;
		}

		if (current_token->token_type == '-') {
			type = NODE_MINUS;
		}

		// if (current_token->token_type == '*') {
		// 	type = NODE_MULTIPLY;
		// }

		// if (current_token->token_type == '/') {
		// 	type = NODE_DIVIDE;
		// }

		if (!type) break;

		node *new_node = bump_alloc(sizeof(node));
		new_node->type = type;
		new_node->left = op_stack;
		op_stack = new_node;
		current_token += 1;
	}

	node *op_node = op_stack;
	op_stack = op_stack->left;

	node *primary_left = primary_stack->left;
	node *primary_right = primary_stack;
	primary_stack = primary_stack->left->left;

	op_node->left = primary_left;
	op_node->right = primary_right;

	top_node = op_node;

	while (op_stack->type != 0) {
		op_node = op_stack;
		op_stack = op_stack->left;

		primary_left = primary_stack;
		primary_stack = primary_stack->left;

		op_node->left = primary_left;
		op_node->right = top_node;

		top_node = op_node;
	}

	return top_node;
}
