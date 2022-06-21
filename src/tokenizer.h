#pragma once
#include "general.h"
#include "memory.h"

typedef enum token_type token_type;

enum token_type {
	TOKEN_INT = 256
};

typedef struct token token;
typedef struct token_list token_list;

struct token {
	u32 token_type;
	u32 value;
};

struct token_list {
	token *tokens;
	u32 count;
};

token_list tokenize(char *code, u32 length);
