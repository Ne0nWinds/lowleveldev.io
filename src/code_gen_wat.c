#include "code_gen_wasm.h"

u8 create_module(u8 *c) {
	static char code[] = "(module";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 create_main_function(u8 *c) {
	static char code[] = " (func (export \"main\") (result i32) ";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 create_code_section(u8 *c, u32 length) {
	return 0;
}

u8 end_module(u8 *c) {
	c[0] = ')';
	c[1] = 0;
	return 2;
}

u8 i32_const(u8 *c, i32 value) {
	static char code[] = "\n\ti32.const ";
	__builtin_memcpy(c, code, len(code) - 1);
	c += len(code) - 1;

	u32 value_length = 0;
	bool is_negative = value < 0;

	if (is_negative) {
		*c++ = '-';
		value *= -1;
	}

	do {
		c[value_length] = value % 10 + '0';
		value_length += 1;
		value /= 10;
	} while (value);

	for (u32 i = 0; i < value_length / 2; ++i) {
		u8 temp = c[i];
		c[i] = c[value_length - 1 - i];
		c[value_length - 1 - i] = temp;
	}

	return len(code) - 1 + value_length + is_negative;
}

u8 i32_add(u8 *c) {
	static char code[] = "\n\ti32.add";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_sub(u8 *c) {
	static char code[] = "\n\ti32.sub";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_mul(u8 *c) {
	static char code[] = "\n\ti32.mul";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_div_s(u8 *c) {
	static char code[] = "\n\ti32.div_s";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_eq(u8 *c) {
	static char code[] = "\n\ti32.eq";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_ne(u8 *c) {
	static char code[] = "\n\ti32.ne";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_gt_s(u8 *c) {
	static char code[] = "\n\ti32.gt_s";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_lt_s(u8 *c) {
	static char code[] = "\n\ti32.lt_s";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_le_s(u8 *c) {
	static char code[] = "\n\ti32.le_s";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 i32_ge_s(u8 *c) {
	static char code[] = "\n\ti32.ge_s";
	__builtin_memcpy(c, code, len(code) - 1);
	return len(code) - 1;
}

u8 end_code_block(u8 *c) {
	c[0] = '\n';
	c[1] = ')';
	return 2;
}
