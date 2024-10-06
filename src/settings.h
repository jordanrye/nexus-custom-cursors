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
} // namespace Settings

#endif /* SETTINGS_H */