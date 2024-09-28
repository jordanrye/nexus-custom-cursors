#ifndef PROCESS_SCANNER_TYPES_H
#define PROCESS_SCANNER_TYPES_H

#include <stdbool.h>

// Signed integer types.
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long int i64;

// Unsigned integer types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

// Floating point types.
typedef float f32;
typedef double f64;

// Boolean type.
//typedef u32 bool;
//#define false 0
//#define true 1

// Two dimension vector.
typedef struct vec2
{
	f32 x;
	f32 y;
} vec2;

// Three dimension vector.
typedef struct vec3
{
	f32 x;
	f32 y;
	f32 z;
} vec3;

// Four dimension vector.
typedef struct vec4
{
	f32 x;
	f32 y;
	f32 z;
	f32 w;
} vec4;

#endif /* PROCESS_SCANNER_TYPES_H */
