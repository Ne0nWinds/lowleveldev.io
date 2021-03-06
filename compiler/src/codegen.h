#pragma once
#include "tokenize.h"
#include "defines.h"

typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NEG,
	ND_NUM,
	ND_EQ,
	ND_NE,
	ND_LT,
	ND_LE,
	ND_GT,
	ND_GE,
	ND_EXPR,
	ND_ASSIGN,
	ND_RETURN,
	ND_VAR,
	ND_BLOCK,
	ND_IF,
	ND_FOR,
	ND_ADDR,
	ND_DEREF,
	ND_FUNCCALL
} NodeKind;

typedef enum {
	TYPE_INT = 1,
	TYPE_PTR,
	TYPE_FUNC,
} TypeKind;

typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Function Function;
typedef struct Type Type;

struct Node {
	NodeKind kind;
	Node *next;
	Node *lhs;
	Node *rhs;
	Node *body;
	Token *tok;
	Type *type;

	// complex statements
	union {
		struct _if { // ND_IF
			Node *condition;
			Node *then;
			Node *els;
		} _if;
		struct _for { // ND_FOR
			Node *init;
			Node *condition;
			Node *increment;
			Node *then;
		} _for;
		struct _func { // ND_FUNCCALL
			char *funcname;
			Node *args;
		} _func;
		Obj *var; // ND_VAR
		int val; // ND_NUM
	};
};

struct Obj {
	char *name;
	int offset;
};

struct Function {
	Node *body;
	char *name;
	Obj *locals;
	unsigned int local_count;
	int stack_size;
};

struct Type {
	TypeKind kind;
	union {
		Type *base; // pointer
		Type *return_type; // function type
		Token *name; // declaration
	};
};

unsigned int gen_expr(unsigned char *c);
Function *ParseTokens();
void print_tree(Node *node);

void add_type(Node *node);
