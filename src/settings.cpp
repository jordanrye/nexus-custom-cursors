#include "settings.h"

#include "cursor_load.h"
#include "shared.h"

#include <filesystem>
#include <fstream>

namespace Settings
{
    std::mutex Mutex;
    std::filesystem::path SettingsPath;
    std::filesystem::path PreviewsPath;
    json Settings = json::object();
    json Previews = json::object();

    void DeserialiseSettings(json object);
    void DeserialisePreviews(json object);
    void LoadCustomCursors();

    json SerialiseSetting(CursorPair cursor);
    json SerialisePreview(CursorPair cursor);
    json SerialisePreview(HiddenCursorPair cursor);
    void QueuePreviews();

    void LoadSettings(const std::filesystem::path& aPath)
    {
        SettingsPath = aPath;

        Mutex.lock();
        {
            try
            {
                std::ifstream file(SettingsPath);
                Settings = json::parse(file);
                file.close();
            }
            catch (json::parse_error& ex)
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "settings.json could not be parsed.");
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", ex.what());
            }
        }
        Mutex.unlock();

        DeserialiseSettings(Settings);
        LoadCustomCursors();
    }

    void LoadPreviews(const std::filesystem::path& aPath)
    {
        PreviewsPath = aPath;

        Mutex.lock();
        {
            try
            {
                std::ifstream file(PreviewsPath);
                Previews = json::parse(file);
                file.close();
            }
            catch (json::parse_error& ex)
            {
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", "previews.json could not be parsed.");
                APIDefs->Log(ELogLevel_WARNING, "CustomCursors", ex.what());
            }
        }
        Mutex.unlock();

        DeserialisePreviews(Previews);
        QueuePreviews();
    }

    void Save()
    {
        /* cursors */
        Settings["cursors"] = json::array();
        Previews["cursors"] = json::array();

        /* special cursors */
        Settings["special_cursors"] = json::array();
        Previews["special_cursors"] = json::array();

        /* hidden cursors */
        Settings["hidden_cursors"] = json::array();
        Previews["hidden_cursors"] = json::array();

        for (auto& cursor : Cursors)
        {
            Settings["cursors"].push_back(SerialiseSetting(cursor));
            Previews["cursors"].push_back(SerialisePreview(cursor));
        }
        Settings["special_cursors"].push_back(SerialiseSetting(CombatCursor));
        Previews["special_cursors"].push_back(SerialisePreview(CombatCursor));
        Settings["special_cursors"].push_back(SerialiseSetting(NexusCursor));
        Previews["special_cursors"].push_back(SerialisePreview(NexusCursor));
        for (auto& cursor : HiddenCursors)
        {
            Settings["hidden_cursors"].push_back(cursor.first);
            Previews["hidden_cursors"].push_back(SerialisePreview(cursor));
        }

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

    void DeserialiseSettings(json object)
    {
        if (!object.is_null())
        {
            if (!object["cursors"].is_null())
            {
                for (auto &cursor : object["cursors"])
                {
                    Hash key = 0;
                    if (!cursor["cursor_id"].is_null()) { cursor["cursor_id"].get_to(key); }
                    if (!cursor["file_path"].is_null()) { cursor["file_path"].get_to(Cursors[key].customFilePath); }
                    if (!cursor["file_format"].is_null()) { cursor["file_format"].get_to(Cursors[key].customFileFormat); }
                    if (!cursor["width"].is_null()) { cursor["width"].get_to(Cursors[key].customWidth); }
                    if (!cursor["height"].is_null()) { cursor["height"].get_to(Cursors[key].customHeight); }
                    if (!cursor["hotspot_x"].is_null()) { cursor["hotspot_x"].get_to(Cursors[key].customHotspotX); }
                    if (!cursor["hotspot_y"].is_null()) { cursor["hotspot_y"].get_to(Cursors[key].customHotspotY); }
                }
            }

            if (!object["special_cursors"].is_null())
            {
                for (auto &cursor : object["special_cursors"])
                {
                    if (E_UID_CURSOR_NEXUS == cursor["cursor_id"])
                    {
                        if (!cursor["file_path"].is_null()) { cursor["file_path"].get_to(NexusCursor.second.customFilePath); }
                        if (!cursor["file_format"].is_null()) { cursor["file_format"].get_to(NexusCursor.second.customFileFormat); }
                        if (!cursor["width"].is_null()) { cursor["width"].get_to(NexusCursor.second.customWidth); }
                        if (!cursor["height"].is_null()) { cursor["height"].get_to(NexusCursor.second.customHeight); }
                        if (!cursor["hotspot_x"].is_null()) { cursor["hotspot_x"].get_to(NexusCursor.second.customHotspotX); }
                        if (!cursor["hotspot_y"].is_null()) { cursor["hotspot_y"].get_to(NexusCursor.second.customHotspotY); }
                    }
                    else if (E_UID_CURSOR_COMBAT == cursor["cursor_id"])
                    {
                        if (!cursor["file_path"].is_null()) { cursor["file_path"].get_to(CombatCursor.second.customFilePath); }
                        if (!cursor["file_format"].is_null()) { cursor["file_format"].get_to(CombatCursor.second.customFileFormat); }
                        if (!cursor["width"].is_null()) { cursor["width"].get_to(CombatCursor.second.customWidth); }
                        if (!cursor["height"].is_null()) { cursor["height"].get_to(CombatCursor.second.customHeight); }
                        if (!cursor["hotspot_x"].is_null()) { cursor["hotspot_x"].get_to(CombatCursor.second.customHotspotX); }
                        if (!cursor["hotspot_y"].is_null()) { cursor["hotspot_y"].get_to(CombatCursor.second.customHotspotY); }
                    }
                }
            }

            if (!object["hidden_cursors"].is_null())
            {
                for (auto &hash : object["hidden_cursors"])
                {
                    HiddenCursors.insert(HiddenCursorPair(hash, CursorPreview()));
                }
            }
        }
    }

    void DeserialisePreviews(json object)
    {
        if (!object.is_null())
        {
            if (!object["cursors"].is_null())
            {
                for (auto &cursor : object["cursors"])
                {
                    Hash key = 0;
                    if (!cursor["cursor_id"].is_null()) { cursor["cursor_id"].get_to(key); }
                    if (!cursor["custom_bits"].is_null()) { cursor["custom_bits"].get_to(Cursors[key].customPreview.bits); }
                    if (!cursor["custom_width"].is_null()) { cursor["custom_width"].get_to(Cursors[key].customPreview.width); }
                    if (!cursor["custom_height"].is_null()) { cursor["custom_height"].get_to(Cursors[key].customPreview.height); }
                    if (!cursor["default_bits"].is_null()) { cursor["default_bits"].get_to(Cursors[key].defaultPreview.bits); }
                    if (!cursor["default_width"].is_null()) { cursor["default_width"].get_to(Cursors[key].defaultPreview.width); }
                    if (!cursor["default_height"].is_null()) { cursor["default_height"].get_to(Cursors[key].defaultPreview.height); }
                }
            }

            if (!object["special_cursors"].is_null())
            {
                for (auto &cursor : object["special_cursors"])
                {
                    if (E_UID_CURSOR_NEXUS == cursor["cursor_id"])
                    {
                        if (!cursor["custom_bits"].is_null()) { cursor["custom_bits"].get_to(NexusCursor.second.customPreview.bits); }
                        if (!cursor["custom_width"].is_null()) { cursor["custom_width"].get_to(NexusCursor.second.customPreview.width); }
                        if (!cursor["custom_height"].is_null()) { cursor["custom_height"].get_to(NexusCursor.second.customPreview.height); }
                        if (!cursor["default_bits"].is_null()) { cursor["default_bits"].get_to(NexusCursor.second.defaultPreview.bits); }
                        if (!cursor["default_width"].is_null()) { cursor["default_width"].get_to(NexusCursor.second.defaultPreview.width); }
                        if (!cursor["default_height"].is_null()) { cursor["default_height"].get_to(NexusCursor.second.defaultPreview.height); }
                    }
                    else if (E_UID_CURSOR_COMBAT == cursor["cursor_id"])
                    {
                        if (!cursor["custom_bits"].is_null()) { cursor["custom_bits"].get_to(CombatCursor.second.customPreview.bits); }
                        if (!cursor["custom_width"].is_null()) { cursor["custom_width"].get_to(CombatCursor.second.customPreview.width); }
                        if (!cursor["custom_height"].is_null()) { cursor["custom_height"].get_to(CombatCursor.second.customPreview.height); }
                        if (!cursor["default_bits"].is_null()) { cursor["default_bits"].get_to(CombatCursor.second.defaultPreview.bits); }
                        if (!cursor["default_width"].is_null()) { cursor["default_width"].get_to(CombatCursor.second.defaultPreview.width); }
                        if (!cursor["default_height"].is_null()) { cursor["default_height"].get_to(CombatCursor.second.defaultPreview.height); }
                    }
                }
            }

            if (!object["hidden_cursors"].is_null())
            {
                for (auto &cursor : object["hidden_cursors"])
                {
                    Hash key = 0;
                    if (!cursor["cursor_id"].is_null()) { cursor["cursor_id"].get_to(key); }
                    if (!cursor["default_bits"].is_null()) { cursor["default_bits"].get_to(HiddenCursors[key].bits); }
                    if (!cursor["default_width"].is_null()) { cursor["default_width"].get_to(HiddenCursors[key].width); }
                    if (!cursor["default_height"].is_null()) { cursor["default_height"].get_to(HiddenCursors[key].height); }
                }
            }
        }
    }

    void LoadCustomCursors()
    {
        for (auto& cursor : Cursors)
        {
            LoadCustomCursor(cursor.second);
        }
        LoadCustomCursor(CombatCursor.second);
        LoadCustomCursor(NexusCursor.second);
    }

    json SerialiseSetting(CursorPair cursor)
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

    json SerialisePreview(CursorPair cursor)
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

    json SerialisePreview(HiddenCursorPair cursor)
    {
        json object{};
        object["cursor_id"] = cursor.first;
        object["default_bits"] = cursor.second.bits;
        object["default_width"] = cursor.second.width;
        object["default_height"] = cursor.second.height;
        return object;
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

} // namespace Settings
