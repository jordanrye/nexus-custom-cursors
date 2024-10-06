#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

#include "shared.h"

#include "cursor_create.h"

static HICON CreateIconFromPNG(const std::string& filename, INT width, INT height, INT hotspotX, INT hotspotY);

HCURSOR CreateCursorFromPNG(const std::string& filename, INT width, INT height, INT hotspotX, INT hotspotY)
{
    return static_cast<HCURSOR>(CreateIconFromPNG(filename, width, height, hotspotX, hotspotY));
}

static HICON CreateIconFromPNG(const std::string& filename, INT width, INT height, INT hotspotX, INT hotspotY)
{
    static const INT STBI_RGBA = 4;

    /* attempt to load image from file */
    INT imageWidth, imageHeight, imageChannels;
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
            unsigned char* colourBits;
            auto colour = CreateDIBSection(hDeviceContext, (BITMAPINFO*)&header, DIB_RGB_COLORS, (void**)&colourBits, nullptr, (DWORD)0);
            ReleaseDC(nullptr, hDeviceContext);

            if (colour)
            {
                /* attempt to create monochrome mask bitmap */
                auto mask = CreateBitmap(width, height, 1, 1, nullptr);

                if (mask)
                {
                    /* copy pixels to colour bitmap */
                    unsigned char* tempPixels = resizedPixels;
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
                    ICONINFO icon{};
                    icon.fIcon = FALSE;
                    icon.xHotspot = hotspotX;
                    icon.yHotspot = hotspotY;
                    icon.hbmMask = mask;
                    icon.hbmColor = colour;

                    /* create icon handle */
                    HICON handle = CreateIconIndirect(&icon);

                    stbi_image_free(resizedPixels);
                    DeleteObject(colour);
                    DeleteObject(mask);

                    return handle;
                }
                else
                {
                    APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to create monochrome mask bitmap.");
                    stbi_image_free(resizedPixels);
                    DeleteObject(colour);
                    return nullptr;
                }
            }
            else
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to create colour bitmap.");
                stbi_image_free(resizedPixels);
                return nullptr;
            }
        }
        else
        {
            APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to resize image.");
            return nullptr;
        }
    }
    else
    {
        APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "Failed to load image.");
        return nullptr;
    }

    return nullptr;
}