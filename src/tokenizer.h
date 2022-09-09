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

typedef enum identifier_type identifier_type;
enum identifier_type {
	IDENTIFIER_FUNC,
	IDENTIFIER_VAR,
	IDENTIFIER_PARAM
};

typedef struct token token;
typedef struct token_list token_list;

struct token {
	u32 type;
	u32 line_number;
	union {
		u32 value;
		identifier identifier;
	};
};

void tokenizer_init(char *code, u32 length);
token current_token();
void advance_token();
void unexpected_token_error(token_type t);
void expected_identifier(identifier_type type);
void redeclaration_error(identifier_type type);
void not_found_error(identifier_type type);
void set_error_msg(char *format_str, ...);
