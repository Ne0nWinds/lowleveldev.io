#include "parser.h"
#include "memory.h"

token *current_token;

bool error_occurred;

typedef struct variable variable;
struct variable {
	char *name;
	u32 length;
	u32 addr;
	variable *left;
	variable *right;
};

static variable *variable_bst;
static u32 stack_pointer;

int string_compare(char *a, char *b, u32 n) {
	for (u32 i = 0; i < n; ++i) {
		if (a[i] != b[i]) return (a < b) ? -1 : 1;
	}
	return 0;
}

variable *add_variable(char *name, u32 length) {

	variable *var = bump_alloc(sizeof(variable));
	var->name = name;
	var->length = length;
	var->addr = stack_pointer;
	stack_pointer -= 4;

	variable *current = variable_bst;
	variable *previous = 0;
	u32 compare_result = 0;

	while (current != 0) {
		previous = current;

		compare_result = string_compare(name, current->name, max(length, current->length));
		if (compare_result == 0) {
			return 0;
		}

		if (compare_result == -1) {
			current = current->left;
		}

		if (compare_result == 1) {
			current = current->right;
		}
	}

	if (compare_result == -1) {
		previous->left = var;
	}

	if (compare_result == 1) {
		previous->right = var;
	}

	if (compare_result == 0) {
		variable_bst = var;
	}

	return var;
};

variable *find_variable(char *name, u32 length) {
	variable *current = variable_bst;

	while (current != 0) {
		u32 compare_result = string_compare(name, current->name, max(length, current->length));

		if (compare_result == 0) {
			return current;
		}

		if (compare_result == -1) {
			current = current->left;
		}

		if (compare_result == 1) {
			current = current->right;
		}
	}

	return 0;
}

node *expr_stmt();
node *expr();
node *code_block();
node *code_block_or_expr_stmt();
void expect_token(token_type c);

node *parse_tokens(token_list tokens) {
	error_occurred = false;
	current_token = tokens.tokens;
	variable_bst = 0;
	stack_pointer = PAGE_SIZE - 4;

	node *block = code_block();

	return (error_occurred) ? 0 : block;
}

void expect_token(token_type t) {
	if (current_token->type == t) {
		current_token += 1;
	} else {
		error_occurred = true;
	}
}

node *code_block() {

	u32 depth = 1;
	expect_token('{');

	node head = {0};
	node *current = &head;

	while (depth > 0 && !error_occurred && current_token->type != 0) {
		while (current_token->type == '{') {
			++depth;
			current_token += 1;
		}

		current->next = expr_stmt();
		current = current->next;

		while (current_token->type == '}') {
			--depth;
			current_token += 1;
		}
	}

	if (depth != 0) {
		error_occurred = true;
		return 0;
	}

	current->next = 0;

	return head.next;
}

node *code_block_or_expr_stmt() {
	if (current_token->type == '{') {
		return code_block();
	}
	return expr_stmt();
}

node *expr_stmt() {
	if (current_token->type == TOKEN_INT_DECL) {
		current_token += 1;

		if (current_token->type != TOKEN_IDENTIFIER) {
			error_occurred = true;
			return 0;
		}

		variable *var = add_variable(current_token->identifier.name, current_token->identifier.length);
		if (!var) {
			error_occurred = true;
			return 0;
		}
		current_token += 1;

		expect_token('=');

		node *declaration = bump_alloc(sizeof(node));
		declaration->type = NODE_INT_DECL;
		declaration->addr = var->addr;
		declaration->right = expr();

		expect_token(';');
		return declaration;
	}

	if (current_token->type == TOKEN_RETURN) {
		current_token += 1;

		node *return_node = bump_alloc(sizeof(node));
		return_node->type = NODE_RETURN;
		return_node->right = expr();

		expect_token(';');
		return return_node;
	}

	node *n = expr();
	expect_token(';');
	return n;
}

enum {
	PRECEDENCE_ASSIGNMENT = 1,
	PRECEDENCE_EQUALITY,
	PRECEDENCE_RELATIONAL,
	PRECEDENCE_ADD,
	PRECEDENCE_MUL,
};

u32 get_precedence(node_type type) {
	if (type == NODE_ASSIGN) return PRECEDENCE_ASSIGNMENT;
	if (type == NODE_LT || type == NODE_LE || type == NODE_GT || type == NODE_GE) return PRECEDENCE_RELATIONAL;
	if (type == NODE_EQ || type == NODE_NE) return PRECEDENCE_EQUALITY;
	if (type == NODE_MULTIPLY || type == NODE_DIVIDE) return PRECEDENCE_MUL;
	return PRECEDENCE_ADD;
}

node *primary() {
	if (current_token->type == '-') {
		current_token += 1;
		node *unary_node = bump_alloc(sizeof(node));
		unary_node->type = NODE_NEGATE;
		unary_node->right = primary();
		return unary_node;
	}

	if (current_token->type == '(') {
		current_token += 1;
		node *primary_node = expr();
		expect_token(')');
		return primary_node;
	}

	if (current_token->type == TOKEN_IDENTIFIER) {
		variable *var = find_variable(current_token->identifier.name, current_token->identifier.length);
		if (!var) goto error;

		node *primary_node = bump_alloc(sizeof(node));
		primary_node->type = NODE_VAR;
		primary_node->addr = var->addr;
		current_token += 1;
		return primary_node;
	}

	if (current_token->type == TOKEN_INT) {
		node *primary_node = bump_alloc(sizeof(node));
		primary_node->type = NODE_INT;
		primary_node->value = current_token->value;
		current_token += 1;
		return primary_node;
	}

error:
	error_occurred = true;
	return 0;
}

node *expr() {
	node *primary_stack = 0;
	node *op_stack = 0;
	node *top_node = 0;

	top_node = primary();
	if (!top_node) return 0;

	while (!error_occurred) {
		node_type type = 0;

		if (current_token->type == '+') {
			type = NODE_PLUS;
		}

		if (current_token->type == '-') {
			type = NODE_MINUS;
		}

		if (current_token->type == '*') {
			type = NODE_MULTIPLY;
		}

		if (current_token->type == '/') {
			type = NODE_DIVIDE;
		}

		if (current_token->type == TOKEN_EQ) {
			type = NODE_EQ;
		}

		if (current_token->type == TOKEN_NE) {
			type = NODE_NE;
		}

		if (current_token->type == '<') {
			type = NODE_LT;
		}

		if (current_token->type == '>') {
			type = NODE_GT;
		}

		if (current_token->type == TOKEN_LE) {
			type = NODE_LE;
		}

		if (current_token->type == TOKEN_GE) {
			type = NODE_GE;
		}

		if (current_token->type == '=') {
			type = NODE_ASSIGN;
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

	if (error_occurred) return 0;

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
