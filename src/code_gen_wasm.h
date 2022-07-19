#pragma once
#include "general.h"

u8 create_module(u8 *c);
u8 end_module(u8 *c);

u8 create_main_function(u8 *c);
u8 create_code_section(u8 *c, u32 length);
u8 end_code_block(u8 *c);

u8 i32_const(u8 *c, i32 value);
u8 i32_add(u8 *c);
u8 i32_sub(u8 *c);
u8 i32_mul(u8 *c);
u8 i32_div_s(u8 *c);

u8 i32_eq(u8 *c);
u8 i32_ne(u8 *c);
u8 i32_gt_s(u8 *c);
u8 i32_lt_s(u8 *c);
u8 i32_le_s(u8 *c);
u8 i32_ge_s(u8 *c);

u8 drop(u8 *c);
u8 wasm_return(u8 *c);

u8 i32_load(u8 *c, u32 alignment, u32 offset);
u8 i32_store(u8 *c, u32 alignment, u32 offset);
