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
			current_token->type = TOKEN_IDENTIFIER;
			u32 length = 1;
			char *start = c;
			current_token->identifier.name = start; // TODO: make cache efficient
			c += 1;

			while (is_alpha_numeric(*c)) {
				length += 1;
				c += 1;
			}
			c -= 1;

			current_token->identifier.length = length;

			if (length == 2 && startswith(start, "if", 2)) {
				current_token->type = TOKEN_IF;
				token_count += 1;
				continue;
			}

			if (length == 3 && startswith(start, "int", 3)) {
				current_token->type = TOKEN_INT_DECL;
				token_count += 1;
				continue;
			}

			if (length == 6 && startswith(start, "return", 6)) {
				current_token->type = TOKEN_RETURN;
				token_count += 1;
				continue;
			}

			if (length == 4 && startswith(start, "else", 4)) {
				current_token->type = TOKEN_ELSE;
				token_count += 1;
				continue;
			}

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
