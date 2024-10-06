#ifndef HASH_H
#define HASH_H

#include "basic_types.h"

uint32_t HashDJB2(const void* buf, uint32_t numBytes);

uint64_t HashFNV1A_64(const void* buf, uint32_t numBytes);

#endif /* HASH_H */
