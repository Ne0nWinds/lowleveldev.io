#include "codegen.h"
#include "tokenize.h"
#include "standard_functions.h"

// TODO: Make Unified Memory Allocator
Node AllNodes[512] = {0};
Node *CurrentNode = AllNodes;
static bool error_parsing = false;

static Type TypeInt = (Type){TYPE_INT, 0};

static int align_to(int n, int align) {
	return (n + align - 1) / align * align;
}

static Node *new_node(NodeKind kind) {
	Node *node = CurrentNode++;
	node->kind = kind;
	node->tok = (struct Token *)CurrentToken();
	return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static Node *new_unary(NodeKind kind, Node *expr) {
	Node *node = new_node(kind);
	node->lhs = expr;
	return node;
}

static Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

#define is_int(n) (n->type->kind == TYPE_INT)
static Node *new_add(Node *lhs, Node *rhs) {
	add_type(lhs);
	add_type(rhs);
	
	// num + num
	if (is_int(lhs) && is_int(rhs))
		return new_binary(ND_ADD, lhs, rhs);

	// ptr + ptr
	if (lhs->type->base && rhs->type->base) {
		error_tok(CurrentToken(), "invalid operands");
		error_parsing = true;
		return 0;
	}

	// num + ptr
	if (!lhs->type->base && rhs->type->base) {
		Node *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}

	// ptr + num
	rhs = new_binary(ND_MUL, rhs, new_num(4));
	return new_binary(ND_ADD, lhs, rhs);
}

static Node *new_sub(Node *lhs, Node *rhs) {
	add_type(lhs);
	add_type(rhs);

	// num - num
	if (is_int(lhs) && is_int(rhs))
		return new_binary(ND_SUB, lhs, rhs);

	// ptr - num
	if (lhs->type->base && is_int(rhs)) {
		rhs = new_binary(ND_MUL, rhs, new_num(4));
		add_type(rhs);
		Node *node = new_binary(ND_SUB, lhs, rhs);
		node->type = lhs->type;
		return node;
	}

	// ptr - ptr
	if (lhs->type->base && rhs->type->base) {
		Node *node = new_binary(ND_SUB, lhs, rhs);
		node->type = &TypeInt;
		return new_binary(ND_DIV, node, new_num(4));
	}

	// num - ptr
	error_tok(CurrentToken(), "invalid operands");
	error_parsing = true;
	return 0;
}

static Obj Locals[512];
Obj *CurrentLocal = 0;
static char *Names[2048] = {0};
char *CurrentChar = (char *)Names;
static Function Functions[128] = {0};
static Function *CurrentFunction;
static unsigned int FunctionCount;
static Type Types[128] = {0};
static Type *CurrentType;

static Node *new_variable(Obj *var) {
	Node *node = new_node(ND_VAR);
	node->var = var;
	return node;
}

static Obj *new_lvar(char *name, unsigned int len) {
	Obj *var = CurrentLocal;
	CurrentLocal += 1;
	memcpy(CurrentChar, name, len);
	var->name = CurrentChar;
	CurrentChar += len;
	*CurrentChar = 0;
	CurrentChar += 1;
	return var;
}

static char *new_funcname(char *name, unsigned int len) {
	char *value = CurrentChar;
	memcpy(CurrentChar, name, len);
	CurrentChar += len;
	*CurrentChar = 0;
	CurrentChar += 1;
	return value;
}

static bool equal(const Token *token, char *op) {
	for (unsigned int i = 0; i < token->len; ++i) {
		if (token->loc[i] != op[i]) return 0;
	}
	return (op[token->len] == '\0');
}

void skip(char *s) {
	const Token *tok = CurrentToken();
	if (!equal(tok, s)) {
		error_tok(tok, "expected '%s' at '%s'", s, tok->loc);
		error_parsing = true;
	}
	NextToken();
}

Function *ParseTokens();
static Node *expr();
static Node *new_expr();
static Node *mul();
static Node *unary();
static Node *primary();
static Node *equality();
static Node *relational();
static Node *add();
static Node *assign();
static Node *complex_expr();
static Node *declaration();
static Node *funcall();
static Function *function();

static Obj *find_var(const Token *tok) {
	for (Obj *var = Locals; var != CurrentLocal; ++var) {
		if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len))
			return var;
	}
	return 0;
}

Function *ParseTokens() {
	CurrentLocal = Locals;
	CurrentChar = (char *)Names;
	ResetCurrentToken();
	error_parsing = false;
	memset(AllNodes, 0, sizeof(AllNodes));
	CurrentFunction = Functions;
	CurrentType = Types;
	while (CurrentToken()->kind && !error_parsing)
		function();
	FunctionCount = CurrentFunction - Functions;
	return (!error_parsing && FunctionCount) ? Functions : 0;
}

static Node *new_expr() {
	Node *node = expr();
	skip(";");
	return node;
}

static Node *expr_or_block() {
	Node *node = 0;

	if (equal(CurrentToken(), ";")) {
		NextToken();
		return node;
	}

	if (equal(CurrentToken(), "if")) {
		node = new_node(ND_IF);
		NextToken();
		skip("(");
		node->_if.condition = expr();
		skip(")");
		node->_if.then = expr_or_block();
		node->_if.els = 0;
		if (equal(CurrentToken(), "else")) {
			NextToken();
			node->_if.els = expr_or_block();
		}
		return node;
	}

	if (equal(CurrentToken(), "for")) {
		node = new_node(ND_FOR);
		NextToken();
		skip("(");
		if (!equal(CurrentToken(), ";"))
			node->_for.init = expr();
		skip(";");
		if (!equal(CurrentToken(), ";"))
			node->_for.condition = expr();
		skip(";");
		if (!equal(CurrentToken(), ")"))
			node->_for.increment = expr();
		skip(")");
		node->_for.then = expr_or_block();
		return node;
	}

	if (equal(CurrentToken(), "while")) {
		node = new_node(ND_FOR);
		NextToken();
		skip("(");
		node->_for.condition = expr();
		skip(")");
		node->_for.then = expr_or_block();
		return node;
	}

	if (equal(CurrentToken(), "return")) {
		NextToken();
		node = new_unary(ND_RETURN, assign());
		skip(";");
		return node;
	}

	if (equal(CurrentToken(), "{")) {
		NextToken();
		node = complex_expr();
	} else {
		node = expr();
		skip(";");
	}
	return node;
}

static Node *complex_expr() {
	Node head = {};
	Node *current = &head;

	while (*CurrentToken()->loc != '}' && !error_parsing) {
		if (equal(CurrentToken(), "{")) {
			NextToken();
			current = current->next = complex_expr();
			continue;
		}

		current = current->next = expr_or_block();
	}

	skip("}");

	Node *node = new_node(ND_BLOCK);
	node->body = head.next;
	return node;
}

static Node *expr() {
	Node *node = 0;

	if (equal(CurrentToken(), ";")) {
		return new_node(ND_BLOCK);
	}

	if (equal(CurrentToken(), "int")) {
		node = declaration();
		return node;
	}

	return assign();
}

static Node *assign() {
	Node *node = equality();
	if (equal(CurrentToken(), "=")) {
		NextToken();
		node = new_binary(ND_ASSIGN, node, assign());
	}
	return node;
}

static Node *add() {
	Node *node = mul();

	for (;;) {
		if (equal(CurrentToken(), "+")) {
			NextToken();
			node = new_add(node, mul());
			continue;
		}

		if (equal(CurrentToken(), "-")) {
			NextToken();
			node = new_sub(node, mul());
			continue;
		}

		return node;
	}
}

static Node *equality() {
	Node *node = relational();

	for (;;) {
		if (equal(CurrentToken(), "==")) {
			NextToken();
			node = new_binary(ND_EQ, node, relational());
			continue;
		}

		if (equal(CurrentToken(), "!=")) {
			NextToken();
			node = new_binary(ND_NE, node, relational());
			continue;
		}

		return node;
	}
}

static Node *relational() {
	Node *node = add();

	for (;;) {
		if (equal(CurrentToken(), "<")) {
			NextToken();
			node = new_binary(ND_LT, node, add());
			continue;
		}
		if (equal(CurrentToken(), "<=")) {
			NextToken();
			node = new_binary(ND_LE, node, add());
			continue;
		}
		if (equal(CurrentToken(), ">")) {
			NextToken();
			node = new_binary(ND_GT, node, add());
			continue;
		}
		if (equal(CurrentToken(), ">=")) {
			NextToken();
			node = new_binary(ND_GE, node, add());
			continue;
		}

		return node;
	}
}

static Node *mul() {
	Node *node = unary();

	for (;;) {
		if (equal(CurrentToken(), "*")) {
			NextToken();
			node = new_binary(ND_MUL, node, unary());
			continue;
		}

		if (equal(CurrentToken(), "/")) {
			NextToken();
			node = new_binary(ND_DIV, node, unary());
			continue;
		}

		return node;
	}
}

static Node *unary() {
	if (equal(CurrentToken(), "+")) {
		NextToken();
		return unary();
	}

	if (equal(CurrentToken(), "-")) {
		NextToken();
		return new_unary(ND_NEG, unary());
	}

	if (equal(CurrentToken(), "&")) {
		NextToken();
		return new_unary(ND_ADDR, unary());
	}

	if (equal(CurrentToken(), "*")) {
		NextToken();
		return new_unary(ND_DEREF, unary());
	}

	return primary();
}

static Node *primary() {
	if (equal(CurrentToken(), "(")) {
		NextToken();
		Node *node = expr();
		skip(")");
		return node;
	}

	if (CurrentToken()->kind == TK_NUM) {
		Node *node = new_num(CurrentToken()->val);
		NextToken();
		return node;
	}

	if (CurrentToken()->kind == TK_IDENTIFIER) {
		if (equal(CurrentToken() + 1, "(")) {
			return funcall();
		}

		Obj *var = find_var(CurrentToken());
		if (!var) {
			error_tok(CurrentToken(), "undefined variable");
			error_parsing = true;
			return 0;
		}
		NextToken();
		return new_variable(var);
	}

	printf("%d\n", CurrentToken()->kind);

	error_tok(CurrentToken(), "expected an expression");
	error_parsing = true;
	return 0;
}

static Node *funcall() {
	Node *node = new_node(ND_FUNCCALL);
	node->tok = (Token *)CurrentToken();
	node->_func.funcname = new_funcname(CurrentToken()->loc, CurrentToken()->len);

	NextToken();
	skip("(");

	Node *current;
	if (!equal(CurrentToken(), ")")) {
		current = node->_func.args = assign();

		while (!equal(CurrentToken(), ")")) {
			skip(",");
			current = current->next = assign();
		}

	}

	NextToken();
	return node;
}

Type *pointer_to(Type *base);

static Type *declspec() {
	skip("int");
	return &TypeInt;
}

static Type *declarator(Type *type) {
	while (equal(CurrentToken(), "*")) {
		type = pointer_to(type);
		NextToken();
	}

	type->name = (Token *)CurrentToken();
	
	return type;
}

static Node *declaration() {
	Type *base_type = declspec();

	Node head = {};
	Node *current = &head;
	bool first_loop = true;

	while (!equal(CurrentToken(), ";")) {
		if (first_loop)
			first_loop = false;
		else
			skip(",");

		Type *type = declarator(base_type);
		Obj *var = new_lvar(CurrentToken()->loc, CurrentToken()->len);
		NextToken();

		if (!equal(CurrentToken(), "="))
			continue;

		Node *lhs = new_variable(var);
		NextToken();
		Node *rhs = assign();
		Node *node = new_binary(ND_ASSIGN, lhs, rhs);
		current = current->next = node;
	}

	Node *node = new_node(ND_BLOCK);
	node->body = head.next;
	return node;
}

static Function *function() {
	Type *type = declspec();
	type = declarator(type);

	Function *fn = CurrentFunction++;
	*fn = (Function){0};
	fn->name = new_funcname(CurrentToken()->loc, CurrentToken()->len);
	NextToken();
	skip("(");
	skip(")");
	skip("{");
	Obj *start = CurrentLocal;
	fn->body = complex_expr();
	fn->locals = start;
	fn->local_count = CurrentLocal - start;
	return fn;
}

Type *pointer_to(Type *base) {
	Type *type = CurrentType++;
	type->kind = TYPE_PTR;
	type->base = base;
	return type;
}

void add_type(Node *node) {
	if (!node) return;
	
	add_type(node->lhs);
	add_type(node->rhs);
	// add_type(node->_for.init);
	// add_type(node->_for.condition);
	// add_type(node->_for.increment);
	// add_type(node->_for.then);

	switch (node->kind) {
		case ND_ADD:
		case ND_SUB:
		case ND_MUL:
		case ND_DIV:
		case ND_NEG:
		case ND_ASSIGN:
			node->type = node->lhs->type;
			return;
		case ND_EQ:
		case ND_NE:
		case ND_GT:
		case ND_GE:
		case ND_LT:
		case ND_LE:
		case ND_VAR:
		case ND_NUM:
		case ND_FUNCCALL:
			node->type = &TypeInt;
			return;
		case ND_ADDR:
			node->type = pointer_to(node->lhs->type);
			return;
		case ND_DEREF:
			if (node->lhs->type->kind == TYPE_PTR)
				node->type = node->lhs->type->base;
			else
				node->type = &TypeInt;
			return;
	}
}

static char *NodeKind_str[] = {
	"ND_ADD",
	"ND_SUB",
	"ND_MUL",
	"ND_DIV",
	"ND_NEG",
	"ND_NUM",
	"ND_EQ",
	"ND_NE",
	"ND_LT",
	"ND_LE",
	"ND_GT",
	"ND_GE",
	"ND_EXPR",
	"ND_ASSIGN",
	"ND_RETURN",
	"ND_VAR",
	"ND_BLOCK",
	"ND_IF",
	"ND_FOR",
	"ND_ADDR",
	"ND_DEREF"
};

static void _print_tree(Node *node) {
	print(NodeKind_str[node->kind]);
	if (node->lhs) print_tree(node->lhs);
	if (node->rhs) print_tree(node->rhs);
}

void print_tree(Node *node) {
	while (node) {
		_print_tree(node);
		node = node->next;
	}
}

static unsigned int total_byte_length;
static unsigned int n_byte_length;
static unsigned char *c = 0;

static Function *current_fn;

static void _gen_expr(Node *node, int *depth) {
	switch (node->kind) {
		case ND_BLOCK: {
			int _depth = 0;
			for (Node *n = node->body; n; n = n->next) {
				_gen_expr(n, &_depth);
				if (n->next && _depth) {
					printf("OP_DROP - depth: %d", _depth);
					c[n_byte_length++] = OP_DROP;
					--_depth;
				}
			}
			*depth = _depth;
			return;
		}
		case ND_NUM: {
			c[n_byte_length++] = OP_I32_CONST;
			EncodeLEB128(c + n_byte_length, node->val, n_byte_length);
			printf("OP_I32_CONST: %d\n", node->val);
			*depth += 1;
			return;
		} break;
		case ND_NEG: {
			_gen_expr(node->lhs, depth);
			c[n_byte_length++] = OP_I32_CONST;
			EncodeLEB128(c + n_byte_length, -1, n_byte_length);
			printf("OP_I32_CONST: %d\n", -1);
			c[n_byte_length++] = OP_I32_MUL;
			print("OP_I32_MUL");
			return;
		} break;
		case ND_VAR: {
			printf("Name: %s", node->var->name);
			c[n_byte_length++] = OP_I32_CONST;
			c[n_byte_length++] = 0;
			printf("OP_I32_CONST: %d\n", 0);
			c[n_byte_length++] = OP_I32_LOAD;
			c[n_byte_length++] = 2;
			EncodeLEB128(c + n_byte_length, node->var->offset, n_byte_length);
			printf("OP_I32_LOAD: %d", node->var->offset);
			*depth += 1;
			return;
		} break;
		case ND_ASSIGN: {
			printf("OP_I32_CONST: %d\n", 0);
			const Node *lhs = node->lhs;
			if (lhs->kind == ND_VAR) {
				c[n_byte_length++] = OP_I32_CONST;
				c[n_byte_length++] = 0;
				_gen_expr(node->rhs, depth);
				c[n_byte_length++] = OP_I32_STORE;
				c[n_byte_length++] = 2;
				EncodeLEB128(c + n_byte_length, lhs->var->offset, n_byte_length);
				printf("OP_I32_STORE: %d", lhs->var->offset);
			} else if (lhs->kind == ND_DEREF) {
				int _depth = 0;
				_gen_expr(lhs->lhs, &_depth);
				_gen_expr(node->rhs, depth);
				c[n_byte_length++] = OP_I32_STORE;
				c[n_byte_length++] = 2;
				c[n_byte_length++] = 0;
			}
			*depth -= 1;
			return;
		} break;
		case ND_RETURN: {
			_gen_expr(node->lhs, depth);
			print("OP_RETURN");
			c[n_byte_length++] = OP_RETURN;
			return;
		}
		case ND_IF: {
			int _depth = 0;
			_gen_expr(node->_if.condition, &_depth);
			c[n_byte_length++] = OP_I32_CONST;
			c[n_byte_length++] = 0;
			c[n_byte_length++] = OP_I32_NE;
			c[n_byte_length++] = OP_IF;
			c[n_byte_length++] = 0x40;
			print("OP_IF");
			_depth = 0;
			_gen_expr(node->_if.then, &_depth);
			if (node->_if.els) {
				_depth = 0;
				c[n_byte_length++] = OP_ELSE;
				_gen_expr(node->_if.els, &_depth);
			} c[n_byte_length++] = OP_END; return;
		}
		case ND_FOR: {
			int _depth = 0;
			if (node->_for.init)
				_gen_expr(node->_for.init, &_depth);
			if (node->_for.condition) {
				c[n_byte_length++] = OP_BLOCK;
				c[n_byte_length++] = 0x40;
				_depth = 0;
				_gen_expr(node->_for.condition, &_depth);
				c[n_byte_length++] = OP_I32_CONST;
				c[n_byte_length++] = 0;
				c[n_byte_length++] = OP_I32_EQ;
				c[n_byte_length++] = OP_BRANCH_IF;
				c[n_byte_length++] = 0;
			}
			c[n_byte_length++] = OP_LOOP;
			c[n_byte_length++] = 0x40;
			_depth = 0;
			if (node->_for.then)
				_gen_expr(node->_for.then, &_depth);
			if (node->_for.increment)
				_gen_expr(node->_for.increment, &_depth);
			if (node->_for.condition) {
				_depth = 0;
				_gen_expr(node->_for.condition, &_depth);
				c[n_byte_length++] = OP_I32_CONST;
				c[n_byte_length++] = 0;
				c[n_byte_length++] = OP_I32_NE;
				c[n_byte_length++] = OP_BRANCH_IF;
				c[n_byte_length++] = 0;
				c[n_byte_length++] = OP_END;
			} else {
				c[n_byte_length++] = OP_BRANCH;
				c[n_byte_length++] = 0;
			}
			c[n_byte_length++] = OP_END;
			return;
		}
		case ND_DEREF: {
			_gen_expr(node->lhs, depth);
			c[n_byte_length++] = OP_I32_LOAD;
			c[n_byte_length++] = 2;
			c[n_byte_length++] = 0;
			return;
		}
		case ND_ADDR: {
			c[n_byte_length++] = OP_I32_CONST;
			EncodeLEB128(c + n_byte_length, node->lhs->var->offset, n_byte_length);
			*depth += 1;
			return;
		}
		case ND_FUNCCALL: {
			printf("%s - 0x%x\n", node->_func.funcname, (unsigned int)node->_func.args);
			Node *current = node->_func.args;
			while (current) {
				int _depth = 0;
				_gen_expr(current, &_depth);
				current = current->next;
			}
			c[n_byte_length++] = OP_CALL;
			for (int i = 0; i < FunctionCount; ++i) {
				if (!strncmp(node->_func.funcname, Functions[i].name, -1)) {
					c[n_byte_length++] = i;
				}
			}
			*depth += 1;
			return;
		}
	}

	_gen_expr(node->lhs, depth);
	_gen_expr(node->rhs, depth);

	switch (node->kind) {
		case ND_ADD: {
			c[n_byte_length++] = OP_I32_ADD;
			print("OP_I32_ADD");
			*depth -= 1;
		} break;
		case ND_SUB: {
			c[n_byte_length++] = OP_I32_SUB;
			print("OP_I32_SUB");
			*depth -= 1;
		} break;
		case ND_MUL: {
			c[n_byte_length++] = OP_I32_MUL;
			print("OP_I32_MUL");
			*depth -= 1;
		} break;
		case ND_DIV: {
			c[n_byte_length++] = OP_I32_DIV_U;
			print("OP_I32_DIV");
			*depth -= 1;
		} break;
		case ND_EQ: {
			c[n_byte_length++] = OP_I32_EQ;
			print("OP_I32_EQ");
			*depth -= 1;
		} break;
		case ND_NE: {
			c[n_byte_length++] = OP_I32_NE;
			print("OP_I32_NE");
			*depth -= 1;
		} break;
		case ND_LT: {
			c[n_byte_length++] = OP_I32_LT_S;
			print("OP_I32_LT");
			*depth -= 1;
		} break;
		case ND_LE: {
			c[n_byte_length++] = OP_I32_LE_S;
			print("OP_I32_LE");
			*depth -= 1;
		} break;
		case ND_GT: {
			c[n_byte_length++] = OP_I32_GT_S;
			print("OP_I32_GT");
			*depth -= 1;
		} break;
		case ND_GE: {
			c[n_byte_length++] = OP_I32_GE_S;
			print("OP_I32_GE");
			*depth -= 1;
		} break;
	}
}

static void assign_lvar_offsets(Function *prog) {
	int offset = 0;
	for (int i = 0; i < prog->local_count; ++i) {
		Obj *var = prog->locals + i;
		offset += 4;
		var->offset = 128 - offset;
	}
	prog->stack_size += align_to(offset, 16);
}

static unsigned long WASM_header(unsigned char *c) {
	c[0] = 0;
	c[1] = 'a';
	c[2] = 's';
	c[3] = 'm';
	c[4] = 1;
	c[5] = 0;
	c[6] = 0;
	c[7] = 0;
	return 8;
}

unsigned int gen_expr(unsigned char *output_code) {
	c = output_code;
	CurrentType = Types;

	c += WASM_header(c);

	c[0] = SECTION_TYPE;
	c[1] = 0x05;
	c[2] = 0x01;
	c[3] = 0x60;
	c[4] = 0;
	c[5] = 1;
	c[6] = VAL_I32;
	c += 7;

	c[0] = SECTION_FUNC;
	c[1] = 0x1 + FunctionCount;
	c[2] = FunctionCount;
	c += 3;
	memset(c, 0, FunctionCount);
	c += FunctionCount;

	c[0] = SECTION_MEMORY;
	c[1] = 0x3;
	c[2] = 0x1;
	c[3] = 0x0;
	c[4] = 0x1;
	c += 5;

	c[0] = SECTION_EXPORT;
	c[1] = 0x08;
	c[2] = 0x01;
	c[3] = 4;
	c[4] = 'm';
	c[5] = 'a';
	c[6] = 'i';
	c[7] = 'n';
	c[8] = EXPORT_FUNC;
	c[9] = 0;
	c += 9;
	n_byte_length = 0;
	unsigned int f = 0;
	for (; f < FunctionCount; ++f) {
		if (!strncmp(Functions[f].name, "main", 4)) {
			EncodeLEB128(c, f, n_byte_length);
			break;
		}
	}
	if (f == FunctionCount && *c == 0) {
		print("Could not find main function");
		return 0;
	}
	c += n_byte_length;

	c[0] = SECTION_CODE;
	// TODO: Allow Variable Length Byte Length
	unsigned char *CodeSectionLength = c + 1;
	c[2] = FunctionCount;
	c += 3;

	for (int i = 0; i < FunctionCount; ++i) {
		Function *f = Functions + i;
		n_byte_length = 0;
		c += 2;

		assign_lvar_offsets(f);

		int depth = 0;
		_gen_expr(f->body, &depth);
		printf("depth: %d", depth);
		if (depth == 0) {
			c[n_byte_length++] = OP_I32_CONST;
			c[n_byte_length++] = 0;
			c[n_byte_length++] = OP_RETURN;
			print("Adding 0 as default result");
			printf("OP_I32_CONST: %d\n", 0);
		}

		c[n_byte_length++] = OP_END;
		c -= 2;

		c[0] = 1 + n_byte_length;
		c[1] = 0x0;
		c += 2 + n_byte_length;
	}

	*CodeSectionLength = c - CodeSectionLength - 1;

	return c - output_code;
}
