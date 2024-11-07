#ifndef SETTINGS_H
#define SETTINGS_H

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <mutex>

using json = nlohmann::json;

namespace Settings
{
    void LoadSettings(const std::filesystem::path& aPath);
    void LoadPreviews(const std::filesystem::path& aPath);
    void Save();
} // namespace Settings

#endif /* SETTINGS_H */