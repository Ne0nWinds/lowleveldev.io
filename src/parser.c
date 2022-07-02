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
	PRECEDENCE_ADD = 1,
	PRECEDENCE_MUL,
	NO_PRECEDENCE
};

u32 get_precedence(node_type type) {
	if (type == NODE_MULTIPLY || type == NODE_DIVIDE) return PRECEDENCE_MUL;
	return PRECEDENCE_ADD;
}

node *expr() {
	node *primary_stack = 0;
	node *op_stack = 0;
	node *top_node = 0;

	if (current_token->token_type == '(') {
		current_token += 1;
		top_node = expr();
		expect_token(')');
	} else if (current_token->token_type == TOKEN_INT) {
		node *primary_node = bump_alloc(sizeof(node));
		primary_node->type = NODE_INT;
		primary_node->value = current_token->value;
		top_node = primary_node;
		current_token += 1;
	} else {
		error_occurred = true;
		return 0;
	}

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

		if (!type) break;

		u32 prev_prec = (op_stack->type != 0) ? get_precedence(op_stack->type) : 0;
		u32 current_prec = get_precedence(type);

		if (current_prec < prev_prec) {
			node *primary = primary_stack;
			primary_stack = primary_stack->right;

			node *op_node = op_stack;
			op_stack = op_stack->right;

			op_node->right = primary;

			node *local_top = op_node;

			while (op_stack) {
				primary = primary_stack;
				primary_stack = primary_stack->right;

				op_node->left = primary;

				op_node = op_stack;
				op_stack = op_stack->right;

				op_node->right = local_top;
				local_top = op_node;
			}

			local_top->left = top_node;
			top_node = local_top;
		}

		node *new_node = bump_alloc(sizeof(node));
		new_node->type = type;
		new_node->right = op_stack;
		op_stack = new_node;
		current_token += 1;

		if (current_token->token_type == '(') {
			current_token += 1;
			top_node = expr();
			expect_token(')');
		} else if (current_token->token_type == TOKEN_INT) {
			node *primary_node = bump_alloc(sizeof(node));
			primary_node->type = NODE_INT;
			primary_node->value = current_token->value;
			primary_node->right = primary_stack;
			primary_stack = primary_node;
			current_token += 1;
		} else break;
	}

	node *primary = primary_stack;
	primary_stack = primary_stack->right;

	node *op_node = op_stack;
	op_stack = op_stack->right;

	op_node->right = primary;

	node *local_top = op_node;

	while (op_stack) {
		primary = primary_stack;
		primary_stack = primary_stack->right;

		op_node->left = primary;

		op_node = op_stack;
		op_stack = op_stack->right;

		op_node->right = local_top;
		local_top = op_node;
	}

	if (local_top) {
		local_top->left = top_node;
		top_node = local_top;
	}

	return top_node;
}
