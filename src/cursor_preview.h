#ifndef CURSOR_PREVIEW_H
#define CURSOR_PREVIEW_H

#include "shared_types.h"

#include <Windows.h>

bool GetBitsFromCursor(HCURSOR hCursor, uint32_t& pWidth, uint32_t& pHeight, std::vector<uint32_t>& pBits);

void CreateResourceFromBits(const uint32_t& pWidth, const uint32_t& pHeight, const std::vector<uint32_t>& pBits, ID3D11ShaderResourceView** ppResource);

#endif /* CURSOR_PREVIEW_H */
