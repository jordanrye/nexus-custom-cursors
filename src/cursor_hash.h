#ifndef CURSOR_HASH_H
#define CURSOR_HASH_H

#include <Windows.h>

#include "shared_types.h"

const Hash HASH_INVALID = 0ULL;

Hash GetCursorHash(HCURSOR hcursor);

#endif /* CURSOR_HASH_H */
