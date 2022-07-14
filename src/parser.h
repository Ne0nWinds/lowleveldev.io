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
};

typedef struct node node;
struct node {
	node_type type;
	node *next;

	u32 value;

	node *left;
	node *right;
};

node *parse_tokens(token_list tokens);
