#include "parser.h"
#include "memory.h"

token *current_token;

bool error_occurred;

static func *current_function;

int string_compare(identifier a, identifier b) {
	u32 n = max(a.length, b.length);
	for (u32 i = 0; i < n; ++i) {
		if (a.name[i] != b.name[i])
			return (a.name[i] < b.name[i]) ? -1 : 1;
	}
	return 0;
}

variable *add_variable(identifier identifier, u32 pointer_indirections) {

	variable_node *var = bump_alloc(sizeof(variable_node));
	var->variable.identifier = identifier;
	var->variable.addr = current_function->locals.stack_pointer;
	var->variable.pointer_indirections = pointer_indirections;
	current_function->locals.stack_pointer += 4;

	variable_node *current = current_function->locals.head;
	variable_node *previous = 0;
	u32 compare_result = 0;

	while (current != 0) {
		previous = current;

		compare_result = string_compare(identifier, current->variable.identifier);
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
		current_function->locals.head = var;
	}

	return &var->variable;
};

variable *find_variable(identifier identifier) {

	variable *arg = current_function->args;
	for (u32 i = 0; i < current_function->arg_count; ++i) {
		u32 compare_result = string_compare(identifier, arg[i].identifier);
		if (!compare_result) {
			return arg + i;
		}
	}

	variable_node *current = current_function->locals.head;

	while (current != 0) {
		u32 compare_result = string_compare(identifier, current->variable.identifier);

		if (compare_result == 0) {
			return &current->variable;
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

static func *function_bst;
static u32 global_function_count;

func *add_function(identifier identifier) {
	func *new_function = bump_alloc(sizeof(func));
	new_function->identifier = identifier;
	new_function->func_idx = global_function_count;
	global_function_count += 1;

	func *current = function_bst;
	func *previous = 0;
	u32 compare_result = 0;

	while (current != 0) {
		previous = current;

		compare_result = string_compare(identifier, current->identifier);
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
		previous->left = new_function;
	}

	if (compare_result == 1) {
		previous->right = new_function;
	}

	if (compare_result == 0) {
		function_bst = new_function;
	}

	return new_function;
}

func *find_function(identifier identifier) {
	func *current = function_bst;

	while (current != 0) {
		u32 compare_result = string_compare(identifier, current->identifier);

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

static node *free_node_stack = 0;

static node *allocate_node() {
	if (free_node_stack == 0)
		return bump_alloc(sizeof(node));

	node *new_node = free_node_stack;
	free_node_stack = free_node_stack->next;
	return new_node;
}

static void free_node(node *n) {
	*n = (node){0};
	if (!free_node_stack) {
		free_node_stack = n;
		return;
	}

	n->next = free_node_stack;
	free_node_stack = n;
}

void function_decl();
node *expr_stmt();
node *expr();
node *decl();
node *primary();
node *code_block();
node *code_block_or_expr_stmt();
node *code_block_or_expr_stmt();
void expect_token(token_type c);

func *parse_tokens(token_list tokens, u32 *function_count) {
	error_occurred = false;
	current_token = tokens.tokens;
	function_bst = 0;
	global_function_count = 0;
	free_node_stack = 0;

	while (current_token->type && !error_occurred) {
		function_decl();
	}

	*function_count = global_function_count;

	return (error_occurred) ? 0 : function_bst;
}

void expect_token(token_type t) {
	if (current_token->type == t) {
		current_token += 1;
	} else {
		error_occurred = true;
	}
}

void function_decl() {
	expect_token(TOKEN_INT_DECL);
	if (current_token->type != TOKEN_IDENTIFIER) goto error;
	func *function = add_function(current_token->identifier);
	current_token += 1;
	if (!function) goto error;

	expect_token('(');

	function->locals.stack_pointer = 0;
	function->arg_count = 0;
	if (current_token->type == TOKEN_INT_DECL) {
		current_token += 1;

		u32 pointer_indirections = 0;
		while (current_token->type == '*') {
			pointer_indirections += 1;
			current_token += 1;
		}

		if (current_token->type != TOKEN_IDENTIFIER) goto error;
		function->arg_count = 1;
		variable *args = bump_alloc(0);
		args[0].identifier = current_token->identifier;
		args[0].addr = function->arg_count * -4;
		args[0].pointer_indirections = 0;
		current_token += 1;

		while (current_token->type == ',') {
			current_token += 1;
			expect_token(TOKEN_INT_DECL);

			u32 pointer_indirections = 0;
			while (current_token->type == '*') {
				pointer_indirections += 1;
				current_token += 1;
			}

			if (current_token->type != TOKEN_IDENTIFIER) goto error;
			variable *arg = args + function->arg_count;
			function->arg_count += 1;
			arg->identifier = current_token->identifier;
			arg->addr = function->arg_count * -4;
			arg->pointer_indirections = pointer_indirections;
			current_token += 1;
		}
		function->args = args;
		bump_alloc(sizeof(variable) * function->arg_count);
	}

	expect_token(')');

	current_function = function;
	function->body = code_block();
	return;
error:
	error_occurred = true;
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
		if (current->next != 0)
			current = current->next;

		while (current_token->type == '}' && depth > 0) {
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

node *decl() {
	if (current_token->type == TOKEN_INT_DECL) {
		current_token += 1;

		u32 pointer_indirections = 0;
		while (current_token->type == '*') {
			pointer_indirections += 1;
			current_token += 1;
		}

		if (current_token->type != TOKEN_IDENTIFIER) {
			error_occurred = true;
			return 0;
		}

		variable *var = add_variable(current_token->identifier, pointer_indirections);
		if (!var) {
			error_occurred = true;
			return 0;
		}
		current_token += 1;

		expect_token('=');

		node *declaration = allocate_node();
		declaration->type = NODE_INT_DECL;
		declaration->var.addr = var->addr;
		declaration->var.pointer_indirections = pointer_indirections;
		declaration->right = expr();

		return declaration;
	}

	error_occurred = true;
	return 0;
}

node *expr_stmt() {

	if (current_token->type == TOKEN_INT_DECL) {
		node *declaration = decl();
		expect_token(';');
		return declaration;
	}

	if (current_token->type == TOKEN_IF) {
		current_token += 1;

		node *if_stmt = allocate_node();
		if_stmt->type = NODE_IF;
		expect_token('(');
		if_stmt->if_stmt.cond = expr();
		expect_token(')');
		if_stmt->if_stmt.body = code_block_or_expr_stmt();

		if (current_token->type == TOKEN_ELSE) {
			current_token += 1;
			if_stmt->if_stmt.else_stmt = code_block_or_expr_stmt();
		}

		return if_stmt;
	}

	if (current_token->type == TOKEN_FOR) {
		current_token += 1;
		node *for_loop = allocate_node();
		for_loop->type = NODE_LOOP;

		expect_token('(');
		if (current_token->type != ';')
			for_loop->loop_stmt.start = (current_token->type == TOKEN_INT_DECL) ? decl() : expr();
		expect_token(';');
		if (current_token->type != ';')
			for_loop->loop_stmt.condition = expr();
		expect_token(';');
		if (current_token->type != ')')
			for_loop->loop_stmt.iteration = expr();
		expect_token(')');

		for_loop->loop_stmt.body = code_block_or_expr_stmt();
		return for_loop;
	}

	if (current_token->type == TOKEN_WHILE) {
		current_token += 1;
		node *while_loop = allocate_node();
		while_loop->type = NODE_LOOP;

		expect_token('(');
		while_loop->loop_stmt.condition = expr();
		expect_token(')');

		while_loop->loop_stmt.body = code_block_or_expr_stmt();
		return while_loop;
	}

	if (current_token->type == TOKEN_DO) {
		current_token += 1;
		node *while_loop = allocate_node();
		while_loop->type = NODE_DO_WHILE;

		while_loop->loop_stmt.body = code_block_or_expr_stmt();

		expect_token(TOKEN_WHILE);
		expect_token('(');
		while_loop->loop_stmt.condition = expr();
		expect_token(')');
		expect_token(';');

		return while_loop;
	}

	if (current_token->type == TOKEN_RETURN) {
		current_token += 1;

		node *return_node = allocate_node();
		return_node->type = NODE_RETURN;
		return_node->right = expr();

		expect_token(';');
		return return_node;
	}

	if (current_token->type == ';') {
		current_token += 1;
		return 0;
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

node *unary() {
	if (current_token->type == '-') {
		current_token += 1;
		node *primary_expr = primary();
		if (primary_expr->type == NODE_INT) {
			primary_expr->value *= -1;
			return primary_expr;
		}

		node *unary_node = allocate_node();
		unary_node->type = NODE_NEGATE;
		unary_node->right = primary_expr;

		return unary_node;
	}

	if (current_token->type == '&' || current_token->type == '*') {

		node head = {0};
		node *current = &head;

		while (current_token->type == '*' || current_token->type == '&') {
			if (current_token->type == '*' && (current_token + 1)->type == '&') {
				current_token += 2;
				continue;
			}
			current = current->right = allocate_node();
			current->type = (current_token->type == '&') ? NODE_ADDRESS : NODE_DEREF;
			current_token += 1;
		}
		current->right = primary();

		return head.right;
	}

	return primary();
}

node *primary() {

	if (current_token->type == '(') {
		current_token += 1;
		node *primary_node = expr();
		expect_token(')');
		return primary_node;
	}

	if (current_token->type == TOKEN_IDENTIFIER) {
		if ((current_token + 1)->type != '(') {
			variable *var = find_variable(current_token->identifier);
			if (!var) goto error;

			node *primary_node = allocate_node();
			primary_node->type = NODE_VAR;
			primary_node->var.addr = var->addr;
			primary_node->var.pointer_indirections = var->pointer_indirections;
			current_token += 1;
			return primary_node;
		} else {
			func *f = find_function(current_token->identifier);
			if (!f) goto error;

			node *function_call = allocate_node();
			function_call->type = NODE_FUNC_CALL;
			function_call->func_call.index = f->func_idx;

			current_token += 1;
			expect_token('(');

			if (f->arg_count) {
				u32 arg_count = 1;

				node *current = expr();
				current->next = 0;

				function_call->func_call.args = current;

				while (!error_occurred && arg_count < f->arg_count && current_token->type == ',') {
					arg_count += 1;
					current_token += 1;
					current = expr();
					current->next = function_call->func_call.args;
					function_call->func_call.args = current;
				}

				if (arg_count != f->arg_count) goto error;
			}

			expect_token(')');

			return function_call;
		}
	}

	if (current_token->type == TOKEN_INT) {
		node *primary_node = allocate_node();
		primary_node->type = NODE_INT;
		primary_node->value = current_token->value;
		current_token += 1;
		return primary_node;
	}

error:
	error_occurred = true;
	return 0;
}

void simplify_node(node *n) {

	if (n->type == NODE_PLUS || n->type == NODE_MINUS) {
		if (n->left->type == NODE_VAR && n->left->var.pointer_indirections || n->left->type == NODE_ADDRESS) {
			node *mul = allocate_node();
			mul->type = NODE_MULTIPLY;

			node *ptr_multipler = allocate_node();
			ptr_multipler->type = NODE_INT;
			ptr_multipler->value = 4;

			mul->left = n->right;
			mul->right = ptr_multipler;

			n->right = mul;
		}
	}

	if (n->type >= NODE_PLUS && n->type <= NODE_LE) {
		if (n->left->type == NODE_INT && n->right->type == NODE_INT) {
			i32 new_value = 0;
			switch (n->type) {
				case NODE_PLUS:
					new_value = n->left->value + n->right->value; break;
				case NODE_MINUS:
					new_value = n->left->value - n->right->value; break;
				case NODE_MULTIPLY:
					new_value = n->left->value * n->right->value; break;
				case NODE_DIVIDE:
					new_value = n->left->value / n->right->value; break;
				case NODE_EQ:
					new_value = n->left->value == n->right->value; break;
				case NODE_NE:
					new_value = n->left->value != n->right->value; break;
				case NODE_GT:
					new_value = n->left->value > n->right->value; break;
				case NODE_LT:
					new_value = n->left->value < n->right->value; break;
				case NODE_GE:
					new_value = n->left->value >= n->right->value; break;
				case NODE_LE:
					new_value = n->left->value <= n->right->value; break;
			}
			n->type = NODE_INT;
			free_node(n->left);
			free_node(n->right);
			n->value = new_value;
		}
	}
}

node *expr() {
	node *primary_stack = 0;
	node *op_stack = 0;
	node *top_node = 0;

	top_node = unary();
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

		if (current_prec <= prev_prec) {
			node *primary = primary_stack;
			primary_stack = primary_stack->next;

			node *op_node = op_stack;
			op_stack = op_stack->next;

			op_node->right = primary;

			node *local_top = op_node;

			while (op_stack && current_prec <= get_precedence(op_stack->type)) {
				primary = primary_stack;
				primary_stack = primary_stack->next;

				op_node->left = primary;
				simplify_node(op_node);

				op_node = op_stack;
				op_stack = op_stack->next;

				op_node->right = local_top;
				local_top = op_node;
			}

			if (!op_stack) {
				local_top->left = top_node;
				top_node = local_top;
				simplify_node(top_node);
			} else {
				local_top->left = primary_stack;
				primary_stack = primary_stack->next;
				local_top->next = primary_stack;
				primary_stack = local_top;
			}
		}

		node *new_node = allocate_node();
		new_node->type = type;
		new_node->next = op_stack;
		op_stack = new_node;
		current_token += 1;

		node *primary_node = unary();
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

		simplify_node(op_node);

		op_node = op_stack;
		op_stack = op_stack->next;

		op_node->right = local_top;
		local_top = op_node;
	}

	if (local_top) {
		local_top->left = top_node;

		top_node = local_top;
	}

	simplify_node(top_node);

	return top_node;
}
