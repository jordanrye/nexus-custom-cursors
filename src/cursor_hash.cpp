#include "cursor_hash.h"

#include "hash.h"

#include <Windows.h>

uint32_t GetCursorHash(HCURSOR hCursor)
{
    uint32_t hash = HASH_INVALID;

    ICONINFO iconInfo{};
    BITMAP bitmap;

    if (NULL != hCursor)
    {
        /* get icon info */
        if (GetIconInfo(hCursor, &iconInfo))
        {
            /* select color bitmap, else select monochrome bitmap */
            HBITMAP hBitmap = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;

            /* get bitmap */
            if (GetObject(hBitmap, sizeof(BITMAP), &bitmap))
            {
                /* allocate buffer */
                uint32_t bufSz = bitmap.bmWidthBytes * bitmap.bmHeight;
                uint8_t* buf = new uint8_t[bufSz];

                // if buf created
                if (buf != nullptr)
                {
                    // read bitmap pixel data
                    uint32_t numBytes = GetBitmapBits(hBitmap, bufSz, buf);

                    if (numBytes > 0)
                    {
                        hash = HashDJB2(buf, numBytes);
                    }

                    // release buffer
                    delete[] buf;
                }

                /* clean-up */
                if (iconInfo.hbmColor != nullptr) { DeleteObject(iconInfo.hbmColor); }
                if (iconInfo.hbmMask != nullptr) { DeleteObject(iconInfo.hbmMask); }
            }
        }
    }

    return hash;
}
