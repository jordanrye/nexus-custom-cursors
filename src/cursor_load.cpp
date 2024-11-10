#include "cursor_load.h"

#include "cursor_create.h"
#include "cursor_preview.h"
#include "shared.h"

#include <string>

static bool IsFileType(const std::string& str, const std::string& ext);

void LoadCustomCursor(CursorPair& cursor)
{
    std::string filepath = std::filesystem::path(cursor.second.customFilePath).string();

    if (!filepath.empty())
    {
        /* create cursor */
        if (IsFileType(filepath, ".png"))
        {
            cursor.second.customCursor = CreateCursorFromPNG(
                filepath,
                cursor.second.customWidth,
                cursor.second.customHeight,
                cursor.second.customHotspotX,
                cursor.second.customHotspotY
            );
            cursor.second.customFileFormat = E_FILE_FORMAT_PNG;
        }
        else if (IsFileType(filepath, ".cur") || IsFileType(filepath, ".ani"))
        {
            cursor.second.customCursor = CreateCursorFromCUR(
                filepath,
                cursor.second.customWidth,
                cursor.second.customHeight
            );
            cursor.second.customFileFormat = E_FILE_FORMAT_CUR;
        }
        else
        {
            cursor.second.customCursor = nullptr;
            cursor.second.customFileFormat = E_FILE_FORMAT_INV;
            APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to load custom cursor (unsupported file type).");
        }

        /* create preview */
        if (cursor.second.customCursor != nullptr)
        {
            GetBitsFromCursor(cursor.second.customCursor, cursor.second.customPreview.width, cursor.second.customPreview.height, cursor.second.customPreview.bits);
            aQueuedPreview.push_back(&cursor.second.customPreview);
        }
    }
}

static bool IsFileType(const std::string& file, const std::string& ext)
{
    return file.compare(file.size() - ext.size(), ext.size(), ext) == 0;
}
