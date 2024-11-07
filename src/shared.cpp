#include "shared.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

AddonAPI* APIDefs = nullptr;
AddonDefinition AddonDef{};
HMODULE hSelf = nullptr;
HWND hClient = nullptr;
ID3D11Device* D3D11Device = nullptr;

std::filesystem::path AddonDir{};
std::filesystem::path IconsDir{};

Mumble::Data* MumbleLink = nullptr;
Mumble::Identity* MumbleIdentity = nullptr;
NexusLinkData* NexusLink = nullptr;

CursorMap Cursors;
HCURSOR hCustomCursor = nullptr;
std::vector<CursorPreview*> aQueuedPreview;
