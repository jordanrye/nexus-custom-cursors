#ifndef SHARED_H
#define SHARED_H

#include <filesystem>
#include <d3d11.h>
#include <dxgi.h>

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

#include "shared_types.h"
#include "utilities.h"

extern AddonAPI* APIDefs;
extern AddonDefinition AddonDef;
extern HMODULE hSelf;
extern HWND hClient;
extern ID3D11Device* D3D11Device;

extern std::filesystem::path AddonDir;
extern std::filesystem::path IconsDir;

extern Mumble::Data* MumbleLink;
extern Mumble::Identity* MumbleIdentity;
extern NexusLinkData* NexusLink;

extern CursorMap cursors;
extern HCURSOR hCustomCursor;

#endif /* SHARED_H */
