#ifndef SETTINGS_H
#define SETTINGS_H

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <mutex>

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