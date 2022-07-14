#include "tokenizer.h"

#define is_whitespace(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')
#define is_digit(c) (c >= '0' && c <= '9')

bool startwith(char *a, char *b) {
	while (*b) {
		if (*a != *b)
			return false;
		a += 1;
		b += 1;
	}
	return true;
}

token_list tokenize(char *code, u32 length) {

	token *tokens = bump_alloc(0);
	u32 token_count = 0;

	for (char *c = code; c - code < length; ++c) {

		if (is_whitespace(*c)) continue;

		token *current_token = tokens + token_count;
		current_token->token_type = *c;

		if (is_digit(*c)) {
			current_token->token_type = TOKEN_INT;
			current_token->value = 0;
			do {
				current_token->value *= 10;
				current_token->value += *c - '0';
				c += 1;
			} while (is_digit(*c));
			c -= 1;
			token_count += 1;
			continue;
		}
		
		if (startwith(c, "==")) {
			current_token->token_type = TOKEN_EQ;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startwith(c, "!=")) {
			current_token->token_type = TOKEN_NE;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startwith(c, ">=")) {
			current_token->token_type = TOKEN_GE;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startwith(c, "<=")) {
			current_token->token_type = TOKEN_LE;
			c += 1;
			token_count += 1;
			continue;
		}

		token_count += 1;
	}

	token_count += 1;

	bump_alloc(token_count * sizeof(token));
	token *last_token = bump_alloc(sizeof(token));
	last_token->token_type = 0;

	return (token_list){ tokens, token_count };
};
