#pragma once
#include "general.h"
#include "memory.h"

typedef enum token_type token_type;

enum token_type {
	TOKEN_INT = 256,
	TOKEN_IDENTIFIER,

	TOKEN_INT_DECL,
	TOKEN_RETURN,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_FOR,
	TOKEN_WHILE,
	TOKEN_DO,

	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_LE,
	TOKEN_GE,
};

typedef struct token token;
typedef struct token_list token_list;

struct token {
	u32 type;
	union {
		u32 value;
		identifier identifier;
	};
};

struct token_list {
	token *tokens;
	u32 count;
};

void tokenizer_init(char *code, u32 length);
token current_token();
void advance_token();
