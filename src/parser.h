#pragma once
#include "tokenizer.h"
#include "general.h"

typedef enum node_type node_type;
enum node_type {
	NODE_INT = 1,

	NODE_PLUS,
	NODE_MINUS,
	NODE_MULTIPLY,
	NODE_DIVIDE,
	NODE_NEGATE,
	NODE_EQ,
	NODE_NE,
	NODE_GT,
	NODE_LT,
	NODE_GE,
	NODE_LE,

	NODE_VAR,
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
		u32 value;
		u32 addr;
	};

	union {
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

node *parse_tokens(token_list tokens);
