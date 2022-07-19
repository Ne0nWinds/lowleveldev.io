#include "tokenizer.h"

#define is_whitespace(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')
#define is_digit(c) (c >= '0' && c <= '9')
#define is_alpha(c) (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
#define is_alpha_numeric(c) (is_alpha(c) || is_digit(c))

bool startswith(char *a, char *b, u32 length) {
	for (u32 i = 0; i < length; ++i) {
		if (a[i] != b[i]) return false;
	}
	return true;
}

token_list tokenize(char *code, u32 length) {

	token *tokens = bump_alloc(0);
	u32 token_count = 0;

	for (char *c = code; c - code < length; ++c) {

		if (is_whitespace(*c)) continue;

		token *current_token = tokens + token_count;
		current_token->type = *c;

		if (is_digit(*c)) {
			current_token->type = TOKEN_INT;
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

		if (is_alpha(*c)) {
			if (startswith(c, "int", 3)) {
				current_token->type = TOKEN_INT_DECL;
				c += 2;
				token_count += 1;
				continue;
			}

			if (startswith(c, "return", 6)) {
				current_token->type = TOKEN_RETURN;
				c += 5;
				token_count += 1;
				continue;
			}

			if (startswith(c, "if", 2)) {
				current_token->type = TOKEN_IF;
				c += 1;
				token_count += 1;
				continue;
			}

			if (startswith(c, "else", 4)) {
				current_token->type = TOKEN_ELSE;
				c += 3;
				token_count += 1;
				continue;
			}

			current_token->type = TOKEN_IDENTIFIER;
			current_token->identifier.length = 1;
			current_token->identifier.name = c; // TODO: make cache efficient
			c += 1;
			while (is_alpha_numeric(*c)) {
				current_token->identifier.length += 1;
				c += 1;
			}
			c -= 1;
			token_count += 1;
			continue;
		}

		if (startswith(c, "==", 2)) {
			current_token->type = TOKEN_EQ;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startswith(c, "!=", 2)) {
			current_token->type = TOKEN_NE;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startswith(c, ">=", 2)) {
			current_token->type = TOKEN_GE;
			c += 1;
			token_count += 1;
			continue;
		}

		if (startswith(c, "<=", 2)) {
			current_token->type = TOKEN_LE;
			c += 1;
			token_count += 1;
			continue;
		}

		token_count += 1;
	}

	token_count += 1;

	bump_alloc(token_count * sizeof(token));
	token *last_token = bump_alloc(sizeof(token));
	last_token->type = 0;

	return (token_list){ tokens, token_count };
};
