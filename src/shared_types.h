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

typedef enum {
    /* settings tabs */
    E_UID_SETTINGS_GENERAL = 0,
    E_UID_SETTINGS_HIDDEN = 1,
    E_UID_RESERVED_2 = 2,
    E_UID_RESERVED_3 = 3,
    E_UID_RESERVED_4 = 4,
    E_UID_RESERVED_5 = 5,
    E_UID_RESERVED_6 = 6,
    E_UID_RESERVED_7 = 7,
    E_UID_RESERVED_8 = 8,
    E_UID_RESERVED_9 = 9,

    /* special cursors */
    E_UID_CURSOR_NEXUS = 10,
    E_UID_CURSOR_COMBAT = 11
} E_UNIQUE_IDENTIFIER;

typedef uint32_t Hash;

struct CursorPreview {
    std::vector<uint32_t> bits;
    ID3D11ShaderResourceView* resource;
    uint32_t width;
    uint32_t height;
};

struct CursorProperties {
    HCURSOR customCursor;
    CursorPreview customPreview;
    CursorPreview defaultPreview;
    std::string customFilePath;
    E_FILE_FORMAT customFileFormat;
    int32_t customWidth;
    int32_t customHeight;
    int32_t customHotspotX;
    int32_t customHotspotY;
};

typedef std::map<Hash, CursorProperties> CursorMap;
typedef std::pair<const Hash, CursorProperties> CursorPair;
typedef std::map<Hash, CursorPreview> HiddenCursorMap;
typedef std::pair<const Hash, CursorPreview> HiddenCursorPair;

#endif /* SHARED_TYPES_H */
