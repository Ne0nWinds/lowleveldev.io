#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define len(arr) (sizeof(arr) / sizeof(*arr))

#define PAGE_SIZE 65536

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

typedef struct identifier identifier;
struct identifier {
	char *name;
	u32 length;
};
