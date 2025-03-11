#include "cursor_preview.h"

#include "shared.h"

#include "stb/stb_image.h"

#include <Windows.h>
#include <d3d11.h>

static void SetHotspotPreviewBits(const uint32_t& pWidth, const uint32_t& pHeight, const uint32_t& pHotspotX, const uint32_t& pHotspotY, std::vector<uint32_t>& pBits);

bool GetBitsFromCursor(HCURSOR hCursor, uint32_t& pWidth, uint32_t& pHeight, std::vector<uint32_t>& pBits, const bool isDrawHotspot)
{
    bool success = false;
    ICONINFO iconInfo{};

    if (GetIconInfo(hCursor, &iconInfo))
    {
        /* select color bitmap, else select monochrome bitmap */
        HBITMAP hBitmap = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;
        BITMAP bitmap{};

        if (GetObject(hBitmap, sizeof(bitmap), &bitmap))
        {
            /* get image dimensions */
            pWidth = bitmap.bmWidth;
            pHeight = bitmap.bmHeight;

            /* set array sizes */
            pBits.resize(pWidth * pHeight);
            uint32_t* bits = new uint32_t[pWidth * pHeight];

            /* create bitmap info header */
            BITMAPINFO bitmapInfo{};
            bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
            bitmapInfo.bmiHeader.biWidth = pWidth;
            bitmapInfo.bmiHeader.biHeight = pHeight;
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
                GetDIBits(hDC_Bitmap, hBitmap, 0, pHeight, bits, &bitmapInfo, DIB_RGB_COLORS);
            }

            /* arrange bits */
            for (uint32_t y = 0U; y < pHeight; y++)
            {
                for (uint32_t x = 0U; x < pWidth; x++)
                {
                    auto pos = (y * pWidth) + x;
                    auto bit = bits[pWidth * (pHeight - 1 - y) + x] | 0x00000000;
                    pBits.at(pos) = bit;
                }
            }

            /* set cursor hotspot bits */
            if (isDrawHotspot)
            {
                SetHotspotPreviewBits(pWidth, pHeight, iconInfo.xHotspot, iconInfo.yHotspot, pBits);
            }

            /* clean-up */
            delete[] bits;
            if (iconInfo.hbmMask != nullptr) { DeleteObject(iconInfo.hbmMask); }
            if (iconInfo.hbmColor != nullptr) { DeleteObject(iconInfo.hbmColor); }
            ReleaseDC(NULL, hDC_Screen);
            DeleteDC(hDC_Bitmap);

            success = true;
        }
    }

    return success;
}

static void SetHotspotPreviewBits(const uint32_t& pWidth, const uint32_t& pHeight, const uint32_t& pHotspotX, const uint32_t& pHotspotY, std::vector<uint32_t>& pBits)
{
    /* draw shadow for hotspot indicator */
    for (uint32_t y = 0U; y < pHeight; y++)
    {
        for (uint32_t x = 0U; x < pWidth; x++)
        {
            if (x == (pHotspotX + 1))
            {
                auto pos = (y * pWidth) + x;
                pBits.at(pos) = 0xFF000000;
            }
            if (y == (pHotspotY + 1))
            {
                auto pos = (y * pWidth) + x;
                pBits.at(pos) = 0xFF000000;
            }
        }
    }
    
    /* draw hotspot indicator */
    for (uint32_t y = 0U; y < pHeight; y++)
    {
        for (uint32_t x = 0U; x < pWidth; x++)
        {
            if (x == pHotspotX)
            {
                auto pos = (y * pWidth) + x;
                pBits.at(pos) = 0xFFFFFFFF;
            }
            if (y == pHotspotY)
            {
                auto pos = (y * pWidth) + x;
                pBits.at(pos) = 0xFFFFFFFF;
            }
        }
    }
}

void CreateResourceFromBits(const uint32_t& pWidth, const uint32_t& pHeight, const std::vector<uint32_t>& pBits, ID3D11ShaderResourceView** ppResource)
{
    if ((pWidth != 0U) && (pHeight != 0U) && !pBits.empty())
    {
        /* create texture description */
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = pWidth;
        texDesc.Height = pHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;

        /* copy bits to subresource */
        D3D11_SUBRESOURCE_DATA subresource{};
        subresource.pSysMem = &pBits[0];
        subresource.SysMemPitch = texDesc.Width * 4;
        subresource.SysMemSlicePitch = 0;

        /* create texture */
        ID3D11Texture2D* pTexture = nullptr;
        D3D11Device->CreateTexture2D(&texDesc, &subresource, &pTexture);

        if (pTexture != nullptr)
        {
            /* create resource description */
            D3D11_SHADER_RESOURCE_VIEW_DESC rsrcDesc{};
            rsrcDesc.Format = texDesc.Format;
            rsrcDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            rsrcDesc.Texture2D.MipLevels = texDesc.MipLevels;
            rsrcDesc.Texture2D.MostDetailedMip = 0;

            /* create resource */
            D3D11Device->CreateShaderResourceView(pTexture, &rsrcDesc, ppResource);
        
            /* clean-up */
            pTexture->Release();
        }
    }
}
