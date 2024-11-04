#include "cursor_create.h"

#include "shared.h"

#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"

static HICON CreateIconFromPNG(const std::string& filename, int32_t width, int32_t height, int32_t hotspotX, int32_t hotspotY);

HCURSOR CreateCursorFromPNG(const std::string& filename, int32_t width, int32_t height, int32_t hotspotX, int32_t hotspotY)
{
    return static_cast<HCURSOR>(CreateIconFromPNG(filename, width, height, hotspotX, hotspotY));
}

static HICON CreateIconFromPNG(const std::string& filename, int32_t width, int32_t height, int32_t hotspotX, int32_t hotspotY)
{
    static const int32_t STBI_RGBA = 4;
    HICON hIcon = nullptr;

    /* attempt to load image from file */
    int32_t imageWidth, imageHeight, imageChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &imageWidth, &imageHeight, &imageChannels, STBI_RGBA);

    if (pixels)
    {
        /* attempt to resize image */
        stbi_uc* resizedPixels = stbir_resize_uint8_linear(pixels, imageWidth, imageHeight, 0, nullptr, width, height, 0, STBIR_RGBA);
        stbi_image_free(pixels);

        if (resizedPixels)
        {
            /* construct bitmap header */
            BITMAPV5HEADER header{};
            header.bV5Size = sizeof(header);
            header.bV5Width = width;
            header.bV5Height = -height;
            header.bV5Planes = 1;
            header.bV5BitCount = 32;
            header.bV5Compression = BI_BITFIELDS;
            header.bV5RedMask = 0x00ff0000;
            header.bV5GreenMask = 0x0000ff00;
            header.bV5BlueMask = 0x000000ff;
            header.bV5AlphaMask = 0xff000000;

            /* attempt to create colour bitmap */
            HDC hDeviceContext;
            hDeviceContext = GetDC(nullptr);
            uint8_t* colourBits;
            auto colour = CreateDIBSection(hDeviceContext, (BITMAPINFO*)&header, DIB_RGB_COLORS, (void**)&colourBits, nullptr, (DWORD)0);
            ReleaseDC(nullptr, hDeviceContext);

            if (colour)
            {
                /* attempt to create monochrome mask bitmap */
                auto mask = CreateBitmap(width, height, 1, 1, nullptr);

                if (mask)
                {
                    /* copy pixels to colour bitmap */
                    uint8_t* tempPixels = resizedPixels;
                    for (int i = 0; i < (width * height); i++)
                    {
                        colourBits[0] = tempPixels[2];
                        colourBits[1] = tempPixels[1];
                        colourBits[2] = tempPixels[0];
                        colourBits[3] = tempPixels[3];
                        colourBits += 4;
                        tempPixels += 4;
                    }

                    /* create icon info */
                    ICONINFO iconInfo{};
                    iconInfo.fIcon = FALSE;
                    iconInfo.xHotspot = hotspotX;
                    iconInfo.yHotspot = hotspotY;
                    iconInfo.hbmMask = mask;
                    iconInfo.hbmColor = colour;

                    /* create icon */
                    hIcon = CreateIconIndirect(&iconInfo);

                    stbi_image_free(resizedPixels);
                    DeleteObject(colour);
                    DeleteObject(mask);
                }
                else
                {
                    APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to create monochrome mask bitmap.");
                    stbi_image_free(resizedPixels);
                    DeleteObject(colour);
                }
            }
            else
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to create colour bitmap.");
                stbi_image_free(resizedPixels);
            }
        }
        else
        {
            APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to resize image.");
        }
    }
    else
    {
        APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to load image.");
    }

    return hIcon;
}
