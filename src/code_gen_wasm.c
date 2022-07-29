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

	I32_EQZ = 0x45,
	I32_EQ = 0x46,
	I32_NE = 0x47,
	I32_LT_S = 0x48,
	I32_LT_U = 0x49,
	I32_GT_S = 0x4A,
	I32_GT_U = 0x4B,

	I32_LE_S = 0x4C,
	I32_LE_U = 0x4D,
	I32_GE_S = 0x4E,
	I32_GE_U = 0x4F,

	I32_LOAD = 0x28,
	I32_STORE = 0x36,

	DROP = 0x1A,
	RETURN = 0x0F,
	BLOCK = 0x2,
	LOOP = 0x3,
	IF = 0x04,
	ELSE = 0x05,
	BR = 0xC,
	BR_IF = 0xD,
};


// leb128 functions can probably be optimized
u8 leb128_encode_len(i32 value) {
	u32 length = 0;
	u8 byte;
	do {
		byte = value & 0x7F;
		value >>= 7;
		++length;
	} while (value);
	if (byte & 0x40) ++length;
	return length;
}

u8 leb128_encode(u8 *c, i32 value) {
	u32 length = 1;
	u8 byte = value & 0x7F;
	*c = byte;
	value >>= 7;

	if (value < 0) {
		while (value != -1) {
			*c++ |= 0x80;
			byte = value & 0x7F;
			value >>= 7;
			*c = byte;
			length += 1;
		}
		if (!(byte & 0x40)) {
			*c++ |= 0x80;
			*c = 127;
			length += 1;
		}
		return length;
	}

	while (value) {
		*c++ |= 0x80;
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

u8 create_wasm_layout(u8 *c, func *bst, u32 function_count) {

	u8 *start = c;

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

	c[0] = SECTION_MEM;
	c[1] = 0x3;
	c[2] = 0x1;
	c[3] = 0x0;
	c[4] = 0x1;
	c += 5;

	*c++ = SECTION_EXPORT;
	u8 *export_length = c++;

	func *function_stack[function_count];
	function_stack[0] = bst;
	u32 function_stack_length = 1;

	*c++ = function_count;
	for (u32 i = 0; i < function_count; ++i) {
		func *f = function_stack[--function_stack_length];
		*c++ = f->length;
		__builtin_memcpy(c, f->name, f->length);
		c += f->length;
		*c++ = 0x0;
		*c++ = 0x0;
		if (f->right) function_stack[function_stack_length++] = f->right;
		if (f->left) function_stack[function_stack_length++] = f->left;
	}

	*export_length = c - export_length - 1;

	return c - start;
}

u8 create_code_section(u8 *c, u32 length) {
	u32 byte_len2 = leb128_encode_len(length + 1);
	u32 byte_len1 = leb128_encode_len(length + 2 + byte_len2);
	u32 calculated_header_byte_length = byte_len2 + byte_len1 + 3;

	__builtin_memcpy(c + calculated_header_byte_length, c, length);

	*c++ = SECTION_CODE;
	c += leb128_encode(c, length + 2 + byte_len2);
	*c++ = 1;
	c += leb128_encode(c, length + 1);
	*c++ = 0; // vec(locals)

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

u8 i32_eqz(u8 *c) {
	*c = I32_EQZ;
	return 1;
}

u8 i32_eq(u8 *c) {
	*c = I32_EQ;
	return 1;
}

u8 i32_ne(u8 *c) {
	*c = I32_NE;
	return 1;
}

u8 i32_gt_s(u8 *c) {
	*c = I32_GT_S;
	return 1;
}

u8 i32_lt_s(u8 *c) {
	*c = I32_LT_S;
	return 1;
}

u8 i32_le_s(u8 *c) {
	*c = I32_LE_S;
	return 1;
}

u8 i32_ge_s(u8 *c) {
	*c = I32_GE_S;
	return 1;
}

u8 i32_store(u8 *c, u32 alignment, u32 offset) {
	*c++ = I32_STORE;
	*c++ = alignment;
	u32 offset_length = leb128_encode(c, offset);
	return 2 + offset_length;
}

u8 i32_load(u8 *c, u32 alignment, u32 offset) {
	*c++ = I32_LOAD;
	*c++ = alignment;
	u32 offset_length = leb128_encode(c, offset);
	return 2 + offset_length;
}

u8 drop(u8 *c) {
	*c = DROP;
	return 1;
}

u8 wasm_return(u8 *c) {
	*c = RETURN;
	return 1;
}

u8 wasm_if(u8 *c) {
	*c++ = IF;
	*c = 0x40;
	return 2;
}

u8 wasm_else(u8 *c) {
	*c = ELSE;
	return 1;
}

u8 br(u8 *c, u32 index) {
	*c++ = BR;
	u32 offset_length = leb128_encode(c, index);
	return 1 + offset_length;
}

u8 br_if(u8 *c, u32 index) {
	*c++ = BR_IF;
	u32 offset_length = leb128_encode(c, index);

	return 1 + offset_length;
}

u8 loop(u8 *c) {
	*c++ = LOOP;
	*c = 0x40;
	return 2;
}

u8 block(u8 *c) {
	*c++ = BLOCK;
	*c = 0x40;
	return 2;
}

u8 end_code_block(u8 *c) {
	*c = 0xB;
	return 1;
}
