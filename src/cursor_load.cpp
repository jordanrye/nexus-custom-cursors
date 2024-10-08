#include <string>

#include "cursor_create.h"
#include "cursor_load.h"

static bool IsFileType(const std::string& str, const std::string& ext);

void LoadCustomCursor(CursorPair& cursor)
{
    std::string filename = IconsDir.string() + cursor.second.customFilePath;

    // cursor 1
    if (IsFileType(filename, ".png"))
    {
        cursor.second.customCursor = CreateCursorFromPNG(
            filename, 
            cursor.second.customWidth, 
            cursor.second.customHeight, 
            cursor.second.customHotspotX, 
            cursor.second.customHotspotY
        );
        cursor.second.customFileFormat = E_FILE_FORMAT_PNG;
    }
    else if (IsFileType(filename, ".cur") || IsFileType(filename, ".ani"))
    {
        HANDLE hImage = LoadImage(
            NULL,
            filename.c_str(),
            IMAGE_CURSOR,
            cursor.second.customWidth,
            cursor.second.customHeight,
            LR_LOADFROMFILE
        );
        cursor.second.customCursor = static_cast<HCURSOR>(hImage);
        cursor.second.customFileFormat = E_FILE_FORMAT_CUR;
    }
    else
    {
        cursor.second.customCursor = NULL;
    }
}

static bool IsFileType(const std::string& str, const std::string& ext)
{
    return str.compare(str.size() - ext.size(), ext.size(), ext) == 0;
}
