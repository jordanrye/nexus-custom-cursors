#ifndef SHARED_H
#define SHARED_H

#include <filesystem>
#include <mutex>

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "utilities.h"

typedef enum {
	E_FILE_FORMAT_INVALID = 0,
	E_FILE_FORMAT_PNG = 1,
	E_FILE_FORMAT_CUR = 2,
	E_FILE_FORMAT_ANI = 3
} E_FILE_FORMAT;

extern AddonAPI* APIDefs;
extern AddonDefinition AddonDef;
extern HMODULE hSelf;
extern HWND hClient;

extern std::filesystem::path Gw2RootDir;
extern std::filesystem::path AddonDir;
extern std::filesystem::path IconsDir;

extern Mumble::Data* MumbleLink;
extern Mumble::Identity* MumbleIdentity;
extern NexusLinkData* NexusLink;

extern HCURSOR hCustomCursor;

#endif /* SHARED_H */
