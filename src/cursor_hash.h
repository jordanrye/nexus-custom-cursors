#ifndef CURSOR_HASH_H
#define CURSOR_HASH_H

#include "basic_types.h"

#include <Windows.h>

const uint32_t HASH_INVALID = 0ULL;

uint32_t GetCursorHash(HCURSOR hCursor);

#endif /* CURSOR_HASH_H */
