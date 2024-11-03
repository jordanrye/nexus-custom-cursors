#ifndef CURSOR_PREVIEW_H
#define CURSOR_PREVIEW_H

#include "shared.h"

void GetBitsFromCursor(HCURSOR hCursor, Image* image);

void CreateResourceFromBits(Image* image);

#endif /* CURSOR_PREVIEW_H */