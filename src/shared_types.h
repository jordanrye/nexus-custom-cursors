#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include <Windows.h>
#include <d3d11.h>
#include <map>
#include <string>
#include <vector>

#include "basic_types.h"

typedef enum {
	E_FILE_FORMAT_INV = 0,
	E_FILE_FORMAT_PNG = 1,
	E_FILE_FORMAT_CUR = 2
} E_FILE_FORMAT;

typedef uint32_t Hash;

typedef struct Image {
	std::vector<uint32_t> bits;
    ID3D11ShaderResourceView* resource;
    uint32_t width;
    uint32_t height;
};

typedef struct CursorProperties {
	HCURSOR customCursor;
    Image preview;
    std::string customFilePath;
    E_FILE_FORMAT customFileFormat;
    INT customWidth;
    INT customHeight;
    INT customHotspotX;
    INT customHotspotY;
};

typedef std::map<Hash, CursorProperties> CursorMap;
typedef std::pair<const Hash, CursorProperties> CursorPair;

#endif /* SHARED_TYPES_H */
