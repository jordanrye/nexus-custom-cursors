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

std::filesystem::path GameDir{};
std::filesystem::path AddonDir{};
std::filesystem::path CacheDir{};
std::filesystem::path IconsDir{};

Mumble::Data* MumbleLink = nullptr;
Mumble::Identity* MumbleIdentity = nullptr;
NexusLinkData* NexusLink = nullptr;

HCURSOR hCustomCursor = nullptr;
std::vector<CursorPreview*> aQueuedPreview;

CursorMap Cursors;
CursorPair CombatCursor = {static_cast<Hash>(E_CURSOR_COMBAT), CursorProperties()};
CursorPair NexusCursor = {static_cast<Hash>(E_CURSOR_NEXUS), CursorProperties()};

Texture* CombatIcon = nullptr;
Texture* NexusIcon = nullptr;
