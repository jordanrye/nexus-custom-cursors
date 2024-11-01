#include <Windows.h>
#include <d3d11.h>

// #define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "cursor_preview.h"

static void GetBitsFromCursor(HCURSOR hCursor, Image* image);
static void CreateResourceFromBits(Image* image);

void GetCursorPreview(HCURSOR hCursor, Image* image)
{
    GetBitsFromCursor(hCursor, image);
    CreateResourceFromBits(image);
}

static void GetBitsFromCursor(HCURSOR hCursor, Image* image)
{
    ICONINFO iconInfo{};
    BITMAP bitmap{};
    BITMAPINFO bitmapInfo{};

    if (GetIconInfo(hCursor, &iconInfo))
    {
        /* select color bitmap, else select monochrome bitmap */
        HBITMAP hBitmap = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;

        if (GetObject(hBitmap, sizeof(bitmap), &bitmap))
        {
            /* get image dimensions */
            image->width = bitmap.bmWidth;
            image->height = bitmap.bmHeight;

            /* set container sizes */
            image->bits.resize(image->width * image->height);
            uint32_t* bits = new uint32_t[image->width * image->height];

            /* construct bitmap info header */
            bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
            bitmapInfo.bmiHeader.biWidth = image->width;
            bitmapInfo.bmiHeader.biHeight = image->height;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;

            /* get device context */
            HDC hDC_Screen = GetDC(NULL);
            HDC hDC_Bitmap = CreateCompatibleDC(hDC_Screen);

            /* get bits */
            if (hDC_Bitmap != nullptr)
            {
                SelectObject(hDC_Bitmap, hBitmap);
                GetDIBits(hDC_Bitmap, hBitmap, 0, image->height, bits, &bitmapInfo, DIB_RGB_COLORS);
            }

            /* arrange bits */
            for (uint32_t y = 0U; y < image->height; y++)
            {
                for (uint32_t x = 0U; x < image->width; x++)
                {
                    auto pos = image->bits.begin() + (y * image->width + x);
                    auto bit = bits[image->width * (image->height - 1 - y) + x] | 0x00000000;
                    image->bits.insert(pos, bit);
                }
            }

            /* clean up */
            delete[] bits;
            if (iconInfo.hbmMask != NULL) { DeleteObject(iconInfo.hbmMask); }
            if (iconInfo.hbmColor != NULL) { DeleteObject(iconInfo.hbmColor); }
            if (hDC_Screen != NULL) { ReleaseDC(NULL, hDC_Screen); }
            if (hDC_Bitmap != NULL) { DeleteDC(hDC_Bitmap); }
        }
    }
}

static void CreateResourceFromBits(Image* image)
{
    /* create texture description */
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = image->width;
    desc.Height = image->height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    /* copy bits to subresource */
    D3D11_SUBRESOURCE_DATA subresource{};
    subresource.pSysMem = &image->bits[0];
    subresource.SysMemPitch = desc.Width * 4;
    subresource.SysMemSlicePitch = 0;

    /* create texture */
    ID3D11Texture2D* texture = nullptr;
    D3D11Device->CreateTexture2D(&desc, &subresource, &texture);

    if (texture)
    {
        /* create resource description */
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        /* create resource */
        D3D11Device->CreateShaderResourceView(texture, &srvDesc, &image->resource);
        
        texture->Release();
    }
}
