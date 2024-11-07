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
        Settings["cursors"] = json::array();
        Previews["cursors"] = json::array();

        for (auto& cursor : Cursors)
        {
            Settings["cursors"].push_back(SerialiseSetting(cursor));
            Previews["cursors"].push_back(SerialisePreview(cursor));
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
                    cursor["cursor_id"].get_to(key);
                    cursor["file_path"].get_to(Cursors[key].customFilePath);
                    cursor["file_format"].get_to(Cursors[key].customFileFormat);
                    cursor["width"].get_to(Cursors[key].customWidth);
                    cursor["height"].get_to(Cursors[key].customHeight);
                    cursor["hotspot_x"].get_to(Cursors[key].customHotspotX);
                    cursor["hotspot_y"].get_to(Cursors[key].customHotspotY);
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
                    cursor["cursor_id"].get_to(key);
                    cursor["raw_bits"].get_to(Cursors[key].preview.bits);
                    cursor["raw_width"].get_to(Cursors[key].preview.width);
                    cursor["raw_height"].get_to(Cursors[key].preview.height);
                }
            }
        }
    }

    void LoadCustomCursors()
    {
        for (auto& cursor : Cursors)
        {
            LoadCustomCursor(cursor);
        }
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
        object["raw_bits"] = cursor.second.preview.bits;
        object["raw_width"] = cursor.second.preview.width;
        object["raw_height"] = cursor.second.preview.height;
        return object;
    }

    void QueuePreviews()
    {
        for (auto& cursor : Cursors)
        {
            aQueuedPreview.push_back(&cursor.second.preview);
        }
    }

} // namespace Settings
