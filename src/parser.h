#pragma once
#include "tokenizer.h"
#include "general.h"

typedef enum node_type node_type;
enum node_type {
	NODE_INT = 1,

	NODE_NEGATE,
	NODE_DEREF,
	NODE_ADDRESS,

	NODE_PLUS,
	NODE_MINUS,
	NODE_MULTIPLY,
	NODE_DIVIDE,
	NODE_EQ,
	NODE_NE,
	NODE_GT,
	NODE_LT,
	NODE_GE,
	NODE_LE,

	NODE_VAR,
	NODE_FUNC_CALL,
	NODE_INT_DECL,
	NODE_ASSIGN,
	NODE_IF,
	NODE_LOOP,
	NODE_DO_WHILE,

	NODE_RETURN,
};

// TODO: perhaps, we can avoid making a tree
typedef struct node node;
struct node {
	node_type type;
	node *next;

	union {
		i32 value;
		struct {
			u32 index;
			u32 stack_pointer;
			node *args;
		} func_call;
		struct {
			u32 addr;
			u32 pointer_indirections;
		} var;
		struct {
			node *left;
			node *right;
		};
		struct {
			node *cond;
			node *body;
			node *else_stmt;
		} if_stmt;
		struct {
			node *start;
			node *condition;
			node *iteration;
			node *body;
		} loop_stmt;
	};
};

typedef struct variable variable;
struct variable {
	identifier identifier;
	i32 addr;
	u32 pointer_indirections;
};

typedef struct variable_node variable_node;
struct variable_node {
	variable variable;
	variable_node *left;
	variable_node *right;
};

typedef struct variable_bst variable_bst;
struct variable_bst {
	variable_node *head;
	u32 stack_pointer;
};

typedef struct func func;
struct func {
	identifier identifier;
	u32 func_idx;
	variable_bst locals;
	variable *args;
	u32 arg_count;
	node *body;
	func *left;
	func *right;
};

func *parse_tokens(token_list tokens, u32 *function_count);
