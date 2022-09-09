#include "memory.h"
#include "tokenizer.h"
#include <stdarg.h>

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

static char error_msg[128];
static char error_msg_len;

__attribute__((export_name("get_error_msg")))
char *get_error_msg() {
	return error_msg;
}

__attribute__((export_name("get_error_msg_len")))
u32 get_error_msg_len() {
	u32 value = error_msg_len;
	error_msg_len = 0;
	return value;
}

static u32 u32_to_str(u32 val, char *str) {
	u32 length = 0;
	do {
		length += 1;
		*str = (val % 10) + '0';
		str += 1;
		val /= 10;
	} while (val);
	return length;
}

// %l -- line number
// %i -- identifier
// %d -- digit
// %s -- string
void set_error_msg(char *format_str, ...) {
	va_list valist;

	va_start(valist, format_str);

	char *c = error_msg;
	for (; *format_str; ++format_str) {
		if (*format_str == '%') {
			format_str += 1;
			switch (*format_str) {
				case 'l': {
					c += u32_to_str(line_number, c);
					continue;
				} break;
				case 'i': {
					identifier s = va_arg(valist, identifier);
					__builtin_memcpy(c, s.name, s.length);
					c += s.length;
					continue;
				} break;
				case 'd': {
					u32 d = va_arg(valist, u32);
					c += u32_to_str(d, c);
					continue;
				} break;
				case 's': {
					char *s = va_arg(valist, char *);
					while (*s) {
						*c++ = *s++;
					}
					continue;
				} break;
			}
		}

		*c++ = *format_str;
	}

	error_msg_len = c - error_msg;

	va_end(valist);
}

void expected_identifier(identifier_type type) {
	char *error_msg_ptr = error_msg;
	const char expected[] = "Expected ";
	__builtin_memcpy(error_msg_ptr, expected, len(expected) - 1);
	error_msg_ptr += len(expected) - 1;

	switch (type) {
		case IDENTIFIER_FUNC: {
			const char token_str[] = "function ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_VAR: {
			const char token_str[] = "variable ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_PARAM: {
			const char token_str[] = "parameter ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
	}

	const char on_line[] = "name on line: ";
	__builtin_memcpy(error_msg_ptr, on_line, len(on_line) - 1);
	error_msg_ptr += len(on_line) - 1;

	error_msg_ptr += u32_to_str(line_number, error_msg_ptr);

	*error_msg_ptr = 0;
	error_msg_len = error_msg_ptr - error_msg;
}

void redeclaration_error(identifier_type type) {
	char *error_msg_ptr = error_msg;
	switch (type) {
		case IDENTIFIER_FUNC: {
			const char token_str[] = "There's already a function with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_VAR: {
			const char token_str[] = "There's already a variable with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_PARAM: {
			const char token_str[] = "There's already a parameter with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
	}

	__builtin_memcpy(error_msg_ptr, _current_token.identifier.name, _current_token.identifier.length);
	error_msg_ptr += _current_token.identifier.length;

	error_msg_len = error_msg_ptr - error_msg;
}

void not_found_error(identifier_type type) {
	char *error_msg_ptr = error_msg;
	switch (type) {
		case IDENTIFIER_FUNC: {
			const char token_str[] = "There's not a function with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_VAR: {
			const char token_str[] = "There's not a variable with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
		case IDENTIFIER_PARAM: {
			const char token_str[] = "There's not a parameter with the name ";
			__builtin_memcpy(error_msg_ptr, token_str, len(token_str) - 1);
			error_msg_ptr += len(token_str) - 1;
			break;
		}
	}

	__builtin_memcpy(error_msg_ptr, _current_token.identifier.name, _current_token.identifier.length);
	error_msg_ptr += _current_token.identifier.length;

	switch (type) {
		case IDENTIFIER_FUNC: {
			const char hint[] = "\nA function needs to be declared before it can be used";
			__builtin_memcpy(error_msg_ptr, hint, len(hint) - 1);
			error_msg_ptr += len(hint) - 1;
			break;
		}
		case IDENTIFIER_VAR: {
			const char hint[] = "\nA variable needs to be declared before it can be used";
			__builtin_memcpy(error_msg_ptr, hint, len(hint) - 1);
			error_msg_ptr += len(hint) - 1;
			break;
		}
		case IDENTIFIER_PARAM: {
			const char hint[] = "\nA parameter needs to be declared before it can be used";
			__builtin_memcpy(error_msg_ptr, hint, len(hint) - 1);
			error_msg_ptr += len(hint) - 1;
			break;
		}
	}

	error_msg_len = error_msg_ptr - error_msg;
}

void tokenizer_init(char *code, u32 length) {
	__builtin_memset(error_msg, 0, len(error_msg));
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
	_current_token.line_number = line_number;

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
