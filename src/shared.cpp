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
std::queue<CursorPreview*> aQueueTexture;
std::queue<Hash> aQueueDelete;
std::queue<Hash> aQueueUnhide;

CursorMap Cursors;
HiddenCursorMap HiddenCursors;
CursorPair NexusCursor(static_cast<Hash>(E_UID_CURSOR_NEXUS), CursorProperties());
CursorPair CombatCursor(static_cast<Hash>(E_UID_CURSOR_COMBAT), CursorProperties());

Texture* CombatIcon = nullptr;
Texture* NexusIcon = nullptr;
