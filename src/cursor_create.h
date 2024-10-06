#ifndef CREATE_CURSOR_H
#define CREATE_CURSOR_H

#include "Windows.h"
#include <string>

HCURSOR CreateCursorFromPNG(const std::string& filename, INT width, INT height, INT hotspotX, INT hotspotY);

#endif /* CREATE_CURSOR_H */
