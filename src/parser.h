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
};

typedef struct node node;
struct node {
	node_type type;

	u32 value;

	node *left;
	node *right;
};

node *parse_tokens(token_list tokens);
