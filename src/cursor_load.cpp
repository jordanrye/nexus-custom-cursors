#include "cursor_load.h"

#include "cursor_create.h"
#include "cursor_preview.h"
#include "shared.h"

#include <string>

static bool IsFileType(const std::string& str, const std::string& ext);
static std::string GetFilePath(std::string path);

void LoadCustomCursor(CursorProperties& cursor)
{
    std::string filepath = GetFilePath(cursor.customFilePath);

    if (!filepath.empty())
    {
        /* create cursor */
        if (IsFileType(filepath, ".png"))
        {
            cursor.customCursor = CreateCursorFromPNG(
                filepath,
                cursor.customWidth,
                cursor.customHeight,
                cursor.customHotspotX,
                cursor.customHotspotY
            );
            cursor.customFileFormat = E_FILE_FORMAT_PNG;
        }
        else if (IsFileType(filepath, ".cur") || IsFileType(filepath, ".ani"))
        {
            cursor.customCursor = CreateCursorFromCUR(
                filepath,
                cursor.customWidth,
                cursor.customHeight
            );
            cursor.customFileFormat = E_FILE_FORMAT_CUR;
        }
        else
        {
            cursor.customCursor = nullptr;
            cursor.customFileFormat = E_FILE_FORMAT_INV;
            APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to load custom cursor (unsupported file type).");
        }

        /* create preview */
        if (cursor.customCursor != nullptr)
        {
            if (GetBitsFromCursor(cursor.customCursor, cursor.customPreview.width, cursor.customPreview.height, cursor.customPreview.bits))
            {
                aQueueTexture.push(&cursor.customPreview);
            }
        }
    }
}

static bool IsFileType(const std::string& file, const std::string& ext)
{
    return file.compare(file.size() - ext.size(), ext.size(), ext) == 0;
}

static std::string GetFilePath(std::string path)
{
    std::filesystem::path filepath = path;
    return (filepath.is_relative() ? (GameDir / filepath) : filepath).string();
}
