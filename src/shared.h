#ifndef SHARED_H
#define SHARED_H

#include "shared_types.h"
#include "utilities.h"

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

#include <d3d11.h>
#include <filesystem>
#include <vector>

extern AddonAPI* APIDefs;
extern AddonDefinition AddonDef;
extern HMODULE hSelf;
extern HWND hClient;
extern ID3D11Device* D3D11Device;

extern std::filesystem::path GameDir;
extern std::filesystem::path AddonDir;
extern std::filesystem::path CacheDir;
extern std::filesystem::path IconsDir;

extern Mumble::Data* MumbleLink;
extern Mumble::Identity* MumbleIdentity;
extern NexusLinkData* NexusLink;

extern HCURSOR hCustomCursor;
extern std::vector<CursorPreview*> aQueuedPreview;

extern CursorMap Cursors;
extern CursorPair CombatCursor;
extern CursorPair NexusCursor;

#endif /* SHARED_H */
