#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <stdbool.h>

/* signed integer types */
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int int64_t;

/* unsigned integer types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;

/* floating point types */
typedef float float32_t;
typedef double float64_t;

/* two dimension vector */
typedef struct vec2
{
    float32_t x;
    float32_t y;
} vec2;

/* three dimension vector */
typedef struct vec3
{
    float32_t x;
    float32_t y;
    float32_t z;
} vec3;

/* four dimension vector */
typedef struct vec4
{
    float32_t x;
    float32_t y;
    float32_t z;
    float32_t w;
} vec4;

#endif /* BASIC_TYPES_H */