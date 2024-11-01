#include "shared.h"

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

CursorMap cursors;
HCURSOR hCustomCursor = nullptr;
