#include "tokenizer.h"

#define is_whitespace(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')
#define is_digit(c) (c >= '0' && c <= '9')

token_list tokenize(char *code, u32 length) {

	token *tokens = bump_alloc(0);
	u32 token_count = 0;

	for (u32 i = 0; i < length; ++i) {
		char c = code[i];

		if (is_whitespace(c)) continue;

		token *current_token = tokens + token_count;
		current_token->token_type = c;

		if (is_digit(c)) {
			current_token->token_type = TOKEN_INT;
			current_token->value = c - '0';
		}

		token_count += 1;
	}

	token_count += 1;

	bump_alloc(token_count * sizeof(token));
	token *last_token = bump_alloc(sizeof(token));
	last_token->token_type = 0;

	return (token_list){ tokens, token_count };
};
