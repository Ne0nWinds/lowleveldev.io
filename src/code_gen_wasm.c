#include "code_gen_wasm.h"
#include <wasm_simd128.h>

enum {
	SECTION_CUSTOM = 0,
	SECTION_TYPE,
	SECTION_IMPORT,
	SECTION_FUNC,
	SECTION_TABLE,
	SECTION_MEM,
	SECTION_GLOBAL,
	SECTION_EXPORT,
	SECTION_START,
	SECTION_ELEM,
	SECTION_CODE,
	SECTION_DATA,
	SECTION_DATA_COUNT,

	VALTYPE_I32 = 0x7F,
	VALTYPE_I64 = 0x7E,
	VALTYPE_F32 = 0x7D,
	VALTYPE_F64 = 0x7C,

	I32_CONST = 0x41,
	I32_ADD = 0x6A,
	I32_SUB = 0x6B,
	I32_MUL = 0x6C,
	I32_DIV_S = 0x6D,
	I32_DIV_U = 0x6E,
	I32_REM_S = 0x6F,
	I32_REM_U = 0x70,
};

u8 leb128_encode(u8 *c, i32 value) {
	u8 byte = value & 0x7F;
	u32 length = 1;
	*c = byte;
	value >>= 7;

	while (value) {
		*c |= 0x80;
		c += 1;
		byte = value & 0x7F;
		value >>= 7;
		*c = byte;
		length += 1;
	}

	if (byte & 0x40) {
		*c++ |= 0x80;
		*c = 0;
		length += 1;
	}

	return length;
}

u8 create_module(u8 *c) {
	c[0] = 0;
	c[1] = 'a';
	c[2] = 's';
	c[3] = 'm';

	c[4] = 1;
	c[5] = 0;
	c[6] = 0;
	c[7] = 0;

	return 8;
}

u8 end_module(u8 *c) {
	return 0;
}

// hard code main function
u8 create_main_function(u8 *c) {
	c[0] = SECTION_TYPE;
	c[1] = 0x5;

	c[2] = 0x1;
	c[3] = 0x60;
	c[4] = 0x0;
	c[5] = 0x1;
	c[6] = VALTYPE_I32;

	c += 7;

	c[0] = SECTION_FUNC;
	c[1] = 0x2;

	c[2] = 0x1;
	c[3] = 0x0;

	c += 4;

	c[0] = SECTION_EXPORT;
	c[1] = 0x08;

	c[2] = 0x1;
	c[3] = 0x4;
	c[4] = 'm';
	c[5] = 'a';
	c[6] = 'i';
	c[7] = 'n';

	c[8] = 0x0;
	c[9] = 0x0;

	c += 10;

	return 21;
}

u8 create_code_section(u8 *c, u32 length) {
	u32 calculated_header_byte_length = 5;

	u8 *x = c + calculated_header_byte_length;
	__builtin_memcpy(x, c, length);

	*c++ = SECTION_CODE;
	*c++ = length + 3;
	*c++ = 1;
	*c++ = length + 1;
	*c++ = 0;

	return calculated_header_byte_length;
}

u8 i32_const(u8 *c, i32 value) {
	*c++ = I32_CONST;
	return leb128_encode(c, value) + 1;
}

u8 i32_add(u8 *c) {
	*c = I32_ADD;
	return 1;
}

u8 i32_sub(u8 *c) {
	*c = I32_SUB;
	return 1;
}

u8 i32_mul(u8 *c) {
	*c = I32_MUL;
	return 1;
}

u8 i32_div_s(u8 *c) {
	*c = I32_DIV_S;
	return 1;
}

u8 end_code_block(u8 *c) {
	*c = 0xB;
	return 1;
}
