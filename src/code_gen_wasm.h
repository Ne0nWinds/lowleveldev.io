#pragma once
#include "general.h"
#include "parser.h"

u8 create_module(u8 *c);
u8 end_module(u8 *c);

u8 create_wasm_layout(u8 *c, func *bst, u32 function_count);
u8 end_code_block(u8 *c);

u8 encode_integer(u8 *c, i32 value);
u8 encode_integer_length(i32 value);

u8 i32_const(u8 *c, i32 value);
u8 i32_add(u8 *c);
u8 i32_sub(u8 *c);
u8 i32_mul(u8 *c);
u8 i32_div_s(u8 *c);

u8 i32_eqz(u8 *c);
u8 i32_eq(u8 *c);
u8 i32_ne(u8 *c);
u8 i32_gt_s(u8 *c);
u8 i32_lt_s(u8 *c);
u8 i32_le_s(u8 *c);
u8 i32_ge_s(u8 *c);

u8 drop(u8 *c);
u8 wasm_return(u8 *c);
u8 wasm_if(u8 *c);
u8 wasm_else(u8 *c);
u8 br(u8 *c, u32 index);
u8 br_if(u8 *c, u32 index);
u8 loop(u8 *c);
u8 block(u8 *c);
u8 call(u8 *c, u32 index);

u8 i32_load(u8 *c, u32 alignment, u32 offset);
u8 i32_store(u8 *c, u32 alignment, u32 offset);

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
	CALL = 0x10,
};
