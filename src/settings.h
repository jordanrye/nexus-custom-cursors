#ifndef SETTINGS_H
#define SETTINGS_H

#include <filesystem>
#include <mutex>

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include "shared.h"

using json = nlohmann::json;

namespace Settings
{
	extern std::mutex Mutex;
	extern std::filesystem::path SettingsPath;
	extern json Settings;

	void Load(const std::filesystem::path& aPath);
	void Save();

	/* Cursor configuration */
	extern std::string CursorFilePath;
	extern E_FILE_FORMAT CursorFileFormat;
	extern INT CursorWidth;
	extern INT CursorHeight;
	extern INT CursorHotspotX;
	extern INT CursorHotspotY;
} // namespace Settings

#endif /* SETTINGS_H */