#include "settings.h"

#include "cursor_load.h"
#include "shared.h"

#include <filesystem>
#include <fstream>

namespace Settings
{
    bool isEnabledNexusCursor = false;
    bool isEnabledCombatCursor = false;
    bool isLinkedWidthHeight = true;
    bool isEnabledHotspotPreview = false;
    bool isToggledDebug = false;
    
    const char* ENABLE_NEXUS_CURSOR = "enable_nexus_cursor";
    const char* ENABLE_COMBAT_CURSOR = "enable_combat_cursor";
    const char* LINK_WIDTH_HEIGHT_INPUTS = "link_width_height_inputs";
    const char* ENABLE_HOTSPOT_PREVIEW = "enable_hotspot_preview";
    const char* TOGGLE_DEBUG_WINDOW = "toggle_debug_window";
    
    std::mutex Mutex;
    std::filesystem::path SettingsPath;
    std::filesystem::path PreviewsPath;
    json Settings = json::object();
    json Previews = json::object();

    bool ParseSettings();
    bool ParsePreviews();
    bool DeserialiseSettings();
    bool DeserialisePreviews();

    void LoadCursors();
    void QueuePreviews();

    void DeserialiseCursorProperties(json object, CursorProperties &properties);
    void DeserialiseCursorPreview(json object, CursorProperties& properties);

    json SerialiseCursorProperties(CursorPair cursor);
    json SerialiseCursorPreview(CursorPair cursor);
    json SerialiseCursorPreview(HiddenCursorPair cursor);

    void Load(const std::filesystem::path& aSettingsPath, const std::filesystem::path& aPreviewsPath)
    {
        SettingsPath = aSettingsPath;
        PreviewsPath = aPreviewsPath;

        /* load settings */
        if (ParseSettings())
        {
            if (DeserialiseSettings())
            {
                LoadCursors();
            }
        }

        /* load previews */
        if (ParsePreviews())
        {
            if (DeserialisePreviews())
            {
                QueuePreviews();
            }
        }
    }

    void Save()
    {
        /* general settings */
        Settings["general"] = json::object();
        {
            auto& general = Settings["general"];
            general[ENABLE_NEXUS_CURSOR] = isEnabledNexusCursor;
            general[ENABLE_COMBAT_CURSOR] = isEnabledCombatCursor;
            general[LINK_WIDTH_HEIGHT_INPUTS] = isLinkedWidthHeight;
            general[ENABLE_HOTSPOT_PREVIEW] = isEnabledHotspotPreview;
            general[TOGGLE_DEBUG_WINDOW] = isToggledDebug;
        }

        /* cursors */
        Settings["cursors"] = json::array();
        Previews["cursors"] = json::array();
        for (auto& cursor : Cursors)
        {
            Settings["cursors"].push_back(SerialiseCursorProperties(cursor));
            Previews["cursors"].push_back(SerialiseCursorPreview(cursor));
        }

        /* special cursors */
        Settings["special_cursors"] = json::array();
        Previews["special_cursors"] = json::array();
        {
            Settings["special_cursors"].push_back(SerialiseCursorProperties(NexusCursor));
            Previews["special_cursors"].push_back(SerialiseCursorPreview(NexusCursor));
            
            Settings["special_cursors"].push_back(SerialiseCursorProperties(CombatCursor));
            Previews["special_cursors"].push_back(SerialiseCursorPreview(CombatCursor));
        }

        /* hidden cursors */
        Settings["hidden_cursors"] = json::array();
        Previews["hidden_cursors"] = json::array();
        for (auto& cursor : HiddenCursors)
        {
            Settings["hidden_cursors"].push_back(cursor.first);
            Previews["hidden_cursors"].push_back(SerialiseCursorPreview(cursor));
        }

        /* write updated configuration to disk */
        Mutex.lock();
        {
            /* save to settings.json */
            std::ofstream file(SettingsPath);
            file << Settings.dump(1, '\t') << std::endl;
            file.close();

            /* save to previews.json */
            std::ofstream previewFile(PreviewsPath);
            previewFile << Previews.dump(1, '\t') << std::endl;
            previewFile.close();
        }
        Mutex.unlock();
    }

    bool ParseSettings()
    {
        bool success = false;

        /* attempt to parse settings.json */
        Mutex.lock();
        {
            try
            {
                std::ifstream file(SettingsPath);
                Settings = json::parse(file);
                file.close();
                success = true;
            }
            catch (json::parse_error& ex)
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "settings.json could not be parsed.");
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", ex.what());
            }
        }
        Mutex.unlock();

        return success;
    }

    bool ParsePreviews()
    {
        bool success = false;

        /* attempt to parse previews.json */
        Mutex.lock();
        {
            try
            {
                std::ifstream file(PreviewsPath);
                Previews = json::parse(file);
                file.close();
                success = true;
            }
            catch (json::parse_error& ex)
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "previews.json could not be parsed.");
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", ex.what());
            }
        }
        Mutex.unlock();

        return success;
    }

    bool DeserialiseSettings()
    {
        bool success = false;

        if (!Settings.is_null())
        {
            if (!Settings["general"].is_null())
            {
                auto& general = Settings["general"];
                if (!general[ENABLE_NEXUS_CURSOR].is_null()) { general[ENABLE_NEXUS_CURSOR].get_to(isEnabledNexusCursor); }
                if (!general[ENABLE_COMBAT_CURSOR].is_null()) { general[ENABLE_COMBAT_CURSOR].get_to(isEnabledCombatCursor); }
                if (!general[LINK_WIDTH_HEIGHT_INPUTS].is_null()) { general[LINK_WIDTH_HEIGHT_INPUTS].get_to(isLinkedWidthHeight); }
                if (!general[ENABLE_HOTSPOT_PREVIEW].is_null()) { general[ENABLE_HOTSPOT_PREVIEW].get_to(isEnabledHotspotPreview); }
                if (!general[TOGGLE_DEBUG_WINDOW].is_null()) { general[TOGGLE_DEBUG_WINDOW].get_to(isToggledDebug); }
            }
            else
            {
                auto& general = Settings["general"];
                general[ENABLE_NEXUS_CURSOR] = isEnabledNexusCursor;
                general[ENABLE_COMBAT_CURSOR] = isEnabledCombatCursor;
                general[LINK_WIDTH_HEIGHT_INPUTS] = isLinkedWidthHeight;
                general[ENABLE_HOTSPOT_PREVIEW] = isEnabledHotspotPreview;
                general[TOGGLE_DEBUG_WINDOW] = isToggledDebug;
            }

            if (!Settings["cursors"].is_null())
            {
                for (auto& object : Settings["cursors"])
                {
                    Hash key = 0;
                    if (!object["cursor_id"].is_null())
                    { 
                        object["cursor_id"].get_to(key);
                        DeserialiseCursorProperties(object, Cursors[key]);
                    }
                }
            }

            if (!Settings["special_cursors"].is_null())
            {
                for (auto& object : Settings["special_cursors"])
                {
                    if (E_UID_CURSOR_NEXUS == object["cursor_id"])
                    {
                        DeserialiseCursorProperties(object, NexusCursor.second);
                    }
                    else if (E_UID_CURSOR_COMBAT == object["cursor_id"])
                    {
                        DeserialiseCursorProperties(object, CombatCursor.second);
                    }
                }
            }

            if (!Settings["hidden_cursors"].is_null())
            {
                for (auto& hash : Settings["hidden_cursors"])
                {
                    HiddenCursors.insert(HiddenCursorPair(hash, CursorPreview()));
                }
            }

            success = true;
        }

        return success;
    }

    bool DeserialisePreviews()
    {
        bool success = false;

        /* attempt to deserialise previews */
        if (!Previews.is_null())
        {
            if (!Previews["cursors"].is_null())
            {
                for (auto& object : Previews["cursors"])
                {
                    Hash key = 0;
                    if (!object["cursor_id"].is_null())
                    { 
                        object["cursor_id"].get_to(key); 
                        DeserialiseCursorPreview(object, Cursors[key]);
                    }
                }
            }

            if (!Previews["special_cursors"].is_null())
            {
                for (auto& object : Previews["special_cursors"])
                {
                    if (E_UID_CURSOR_NEXUS == object["cursor_id"])
                    {
                        DeserialiseCursorPreview(object, NexusCursor.second);
                    }
                    else if (E_UID_CURSOR_COMBAT == object["cursor_id"])
                    {
                        DeserialiseCursorPreview(object, CombatCursor.second);
                    }
                }
            }

            if (!Previews["hidden_cursors"].is_null())
            {
                for (auto& object : Previews["hidden_cursors"])
                {
                    Hash key = 0;
                    if (!object["cursor_id"].is_null()) { object["cursor_id"].get_to(key); }
                    if (!object["default_bits"].is_null()) { object["default_bits"].get_to(HiddenCursors[key].bits); }
                    if (!object["default_width"].is_null()) { object["default_width"].get_to(HiddenCursors[key].width); }
                    if (!object["default_height"].is_null()) { object["default_height"].get_to(HiddenCursors[key].height); }
                }
            }

            success = true;
        }

        return success;
    }

    void LoadCursors()
    {
        for (auto& cursor : Cursors)
        {
            LoadCustomCursor(cursor.second, isEnabledHotspotPreview);
        }
        LoadCustomCursor(CombatCursor.second, isEnabledHotspotPreview);
        LoadCustomCursor(NexusCursor.second, isEnabledHotspotPreview);
    }

    void QueuePreviews()
    {
        for (auto& cursor : Cursors)
        {
            aQueueTexture.push(&cursor.second.customPreview);
            aQueueTexture.push(&cursor.second.defaultPreview);
        }
        for (auto& cursor : HiddenCursors)
        {
            aQueueTexture.push(&cursor.second);
        }
    }

    void DeserialiseCursorProperties(json object, CursorProperties &properties)
    {
        if (!object["file_path"].is_null()) { object["file_path"].get_to(properties.customFilePath); }
        if (!object["file_format"].is_null()) { object["file_format"].get_to(properties.customFileFormat); }
        if (!object["width"].is_null()) { object["width"].get_to(properties.customWidth); }
        if (!object["height"].is_null()) { object["height"].get_to(properties.customHeight); }
        if (!object["hotspot_x"].is_null()) { object["hotspot_x"].get_to(properties.customHotspotX); }
        if (!object["hotspot_y"].is_null()) { object["hotspot_y"].get_to(properties.customHotspotY); }
    }

    void DeserialiseCursorPreview(json object, CursorProperties &properties)
    {
        if (!object["custom_bits"].is_null()) { object["custom_bits"].get_to(properties.customPreview.bits); }
        if (!object["custom_width"].is_null()) { object["custom_width"].get_to(properties.customPreview.width); }
        if (!object["custom_height"].is_null()) { object["custom_height"].get_to(properties.customPreview.height); }
        if (!object["default_bits"].is_null()) { object["default_bits"].get_to(properties.defaultPreview.bits); }
        if (!object["default_width"].is_null()) { object["default_width"].get_to(properties.defaultPreview.width); }
        if (!object["default_height"].is_null()) { object["default_height"].get_to(properties.defaultPreview.height); }
    }

    json SerialiseCursorProperties(CursorPair cursor)
    {
        json object{};
        object["cursor_id"] = cursor.first;
        object["file_path"] = cursor.second.customFilePath;
        object["file_format"] = cursor.second.customFileFormat;
        object["width"] = cursor.second.customWidth;
        object["height"] = cursor.second.customHeight;
        object["hotspot_x"] = cursor.second.customHotspotX;
        object["hotspot_y"] = cursor.second.customHotspotY;
        return object;
    }

    json SerialiseCursorPreview(CursorPair cursor)
    {
        json object{};
        object["cursor_id"] = cursor.first;
        object["custom_bits"] = cursor.second.customPreview.bits;
        object["custom_width"] = cursor.second.customPreview.width;
        object["custom_height"] = cursor.second.customPreview.height;
        object["default_bits"] = cursor.second.defaultPreview.bits;
        object["default_width"] = cursor.second.defaultPreview.width;
        object["default_height"] = cursor.second.defaultPreview.height;
        return object;
    }

    json SerialiseCursorPreview(HiddenCursorPair cursor)
    {
        json object{};
        object["cursor_id"] = cursor.first;
        object["default_bits"] = cursor.second.bits;
        object["default_width"] = cursor.second.width;
        object["default_height"] = cursor.second.height;
        return object;
    }

} // namespace Settings
