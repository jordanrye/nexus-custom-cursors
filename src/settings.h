#ifndef SETTINGS_H
#define SETTINGS_H

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <filesystem>
#include <mutex>

using json = nlohmann::json;

namespace Settings
{
    void Load(const std::filesystem::path& aSettingsPath, const std::filesystem::path& aPreviewsPath);
    void Save();
    
    /* state flags */
    extern bool isEnabledNexusCursor;
    extern bool isEnabledCombatCursor;
    extern bool isLinkedWidthHeight;
    extern bool isEnabledHotspotPreview;
    extern bool isToggledDebug;
} // namespace Settings

#endif /* SETTINGS_H */