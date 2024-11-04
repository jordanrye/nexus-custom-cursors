#ifndef CURSOR_CREATE_H
#define CURSOR_CREATE_H

#include "basic_types.h"

#include <Windows.h>
#include <string>

HCURSOR CreateCursorFromPNG(const std::string& filename, int32_t width, int32_t height, int32_t hotspotX, int32_t hotspotY);

#endif /* CURSOR_CREATE_H */
