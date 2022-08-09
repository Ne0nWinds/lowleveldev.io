#include "code_gen_wasm.h"
#include <wasm_simd128.h>

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

u8 encode_integer(u8 *c, i32 value) {
	return leb128_encode(c, value);
}
u8 encode_integer_length(i32 value) {
	return leb128_encode_len(value);
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
	c[1] = 1 + function_count;
	c[2] = function_count;
	c += 3;
	for (u32 i = 0; i < function_count; ++i) {
		*c++ = 0;
	}

	c[0] = SECTION_MEM;
	c[1] = 0x3;
	c[2] = 0x1;
	c[3] = 0x0;
	c[4] = 0x1;
	c += 5;

	*c++ = SECTION_GLOBAL;
	*c++ = 8;
	*c++ = 0x1;
	*c++ = VALTYPE_I32;
	*c++ = 0x1;
	*c++ = I32_CONST;
	c += encode_integer(c, PAGE_SIZE - 4); // length of 3
	*c++ = 0xB;

	*c++ = SECTION_EXPORT;
	u8 *export_length = c++;

	func *function_stack[function_count];
	function_stack[0] = bst;
	u32 function_stack_length = 1;

	*c++ = function_count;
	for (u32 i = 0; i < function_count; ++i) {
		func *f = function_stack[--function_stack_length];
		*c++ = f->identifier.length;
		__builtin_memcpy(c, f->identifier.name, f->identifier.length);
		c += f->identifier.length;
		*c++ = 0x0;
		*c++ = f->func_idx;
		if (f->right) function_stack[function_stack_length++] = f->right;
		if (f->left) function_stack[function_stack_length++] = f->left;
	}

	*export_length = c - export_length - 1;

	return c - start;
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

u8 call(u8 *c, u32 index) {
	*c++ = CALL;
	return leb128_encode(c, index) + 1;
}

u8 end_code_block(u8 *c) {
	*c = 0xB;
	return 1;
}
