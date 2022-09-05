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

static char *c;
static char *src;
static u32 code_length;
static u32 line_number;
static token _current_token;

void tokenizer_init(char *code, u32 length) {
	c = src = code;
	code_length = length;
	line_number = 1;
	advance_token();
}

token current_token() {
	return _current_token;
}

void advance_token() {
	for (; c - src < code_length; c += 1) {

		if (*c == '\n') line_number += 1;
		if (is_whitespace(*c)) continue;

		if (*c == '/' && *(c + 1) == '/') {
			c += 2;
			while (*c && *c != '\n') c += 1;
			c -= 1;
			continue;
		}

		if (*c == '/' && *(c + 1) == '*') {
			c += 2;
			while (*c && *c != '*' && *(c + 1) != '/') c += 1;
			c += 1;
			continue;
		}

		break;
	}

	_current_token = (token){0};
	_current_token.type = *c;

	if (is_digit(*c)) {
		_current_token.type = TOKEN_INT;
		_current_token.value = 0;
		do {
			_current_token.value *= 10;
			_current_token.value += *c - '0';
			c += 1;
		} while (is_digit(*c));
		return;
	}

	if (is_alpha(*c)) {
		_current_token.type = TOKEN_IDENTIFIER;
		u32 length = 1;
		char *start = c;
		_current_token.identifier.name = start; // TODO: make cache efficient
		c += 1;

		while (is_alpha_numeric(*c)) {
			length += 1;
			c += 1;
		}

		_current_token.identifier.length = length;

		if (length == 2 && startswith(start, "if", 2)) {
			_current_token.type = TOKEN_IF;
			return;
		}

		if (length == 2 && startswith(start, "do", 2)) {
			_current_token.type = TOKEN_DO;
			return;
		}

		if (length == 3 && startswith(start, "int", 3)) {
			_current_token.type = TOKEN_INT_DECL;
			return;
		}

		if (length == 3 && startswith(start, "for", 3)) {
			_current_token.type = TOKEN_FOR;
			return;
		}

		if (length == 4 && startswith(start, "else", 4)) {
			_current_token.type = TOKEN_ELSE;
			return;
		}

		if (length == 5 && startswith(start, "while", 5)) {
			_current_token.type = TOKEN_WHILE;
			return;
		}

		if (length == 6 && startswith(start, "return", 6)) {
			_current_token.type = TOKEN_RETURN;
			return;
		}

		return;
	}

	if (startswith(c, "==", 2)) {
		_current_token.type = TOKEN_EQ;
		c += 2;
		return;
	}

	if (startswith(c, "!=", 2)) {
		_current_token.type = TOKEN_NE;
		c += 2;
		return;
	}

	if (startswith(c, ">=", 2)) {
		_current_token.type = TOKEN_GE;
		c += 2;
		return;
	}

	if (startswith(c, "<=", 2)) {
		_current_token.type = TOKEN_LE;
		c += 2;
		return;
	}

	c += 1;
}
