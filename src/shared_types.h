#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include "basic_types.h"

#include <Windows.h>
#include <d3d11.h>
#include <map>
#include <string>
#include <vector>

typedef enum {
    E_FILE_FORMAT_INV = 0,
    E_FILE_FORMAT_PNG = 1,
    E_FILE_FORMAT_CUR = 2,
    E_FILE_FORMAT_X11 = 3
} E_FILE_FORMAT;

typedef uint32_t Hash;

struct CursorPreview {
    std::vector<uint32_t> bits;
    ID3D11ShaderResourceView* resource;
    uint32_t width;
    uint32_t height;
};

struct CursorProperties {
    HCURSOR customCursor;
    CursorPreview preview;
    std::string customFilePath;
    E_FILE_FORMAT customFileFormat;
    int32_t customWidth;
    int32_t customHeight;
    int32_t customHotspotX;
    int32_t customHotspotY;
};

typedef std::map<Hash, CursorProperties> CursorMap;
typedef std::pair<const Hash, CursorProperties> CursorPair;

#endif /* SHARED_TYPES_H */
