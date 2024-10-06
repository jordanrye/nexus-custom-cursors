#include <Windows.h>

#include "cursor_hash.h"
#include "hash.h"

Hash GetCursorHash(HCURSOR hCursor)
{
	Hash hash = HASH_INVALID;

	ICONINFO icon{};
	unsigned char* buf;
	BITMAP bitmap;
	HBITMAP hBitmap;

	if (NULL != hCursor)
	{
		/* get icon info */
		if (GetIconInfo(hCursor, &icon))
		{
			/* select color bitmap, else select mask bitmap */
			hBitmap = icon.hbmColor ? icon.hbmColor : icon.hbmMask;

			/* get icon bitmap */
			if (GetObject(hBitmap, sizeof(BITMAP), &bitmap))
			{
				/* allocate buffer */
				uint32_t bufSz = bitmap.bmWidthBytes * bitmap.bmHeight;
				buf = new unsigned char[bufSz];

				// if buf created
				if( buf != nullptr )
				{
					// read icon bitmap pixel data
					uint32_t numBytes = GetBitmapBits(hBitmap, bufSz, buf);

					if (numBytes > 0)
					{
						hash = HashDJB2(buf, numBytes);
					}

					// release buffer
					delete[] buf;
				}

				if (icon.hbmColor)
				{
					DeleteObject(icon.hbmColor); 
				}
				if (icon.hbmMask)
				{
					DeleteObject(icon.hbmMask);	
				}
			}
		}
	}

	return hash;
}
