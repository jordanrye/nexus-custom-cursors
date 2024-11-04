#include "settings.h"

#include "cursor_load.h"
#include "shared.h"

#include <filesystem>
#include <fstream>

namespace Settings
{
	std::mutex Mutex;
	std::filesystem::path SettingsPath;
	json Settings = json::object();

	static void LoadCustomCursors();

	void Load(const std::filesystem::path& aPath)
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

		if (!Settings.is_null())
		{
			if (!Settings["cursors"].is_null())
			{
				for (auto &cursor : Settings["cursors"])
				{
					Hash key = 0;
					cursor["cursor_id"].get_to(key);
					cursor["file_path"].get_to(cursors[key].customFilePath);
					cursor["file_format"].get_to(cursors[key].customFileFormat);
					cursor["width"].get_to(cursors[key].customWidth);
					cursor["height"].get_to(cursors[key].customHeight);
					cursor["hotspot_x"].get_to(cursors[key].customHotspotX);
					cursor["hotspot_y"].get_to(cursors[key].customHotspotY);
					cursor["preview"].get_to(cursors[key].preview.bits);
					cursor["preview_width"].get_to(cursors[key].preview.width);
					cursor["preview_height"].get_to(cursors[key].preview.height);
				}
			}
		}

		/* load cursors into CursorMap */
		LoadCustomCursors();
	}

	void Save()
	{
		Settings["cursors"] = json::array();

		for(auto& cursor : cursors)
		{
			json obj{};
			obj["cursor_id"] = cursor.first;
			obj["file_path"] = cursor.second.customFilePath;
			obj["file_format"] = cursor.second.customFileFormat;
			obj["width"] = cursor.second.customWidth;
			obj["height"] = cursor.second.customHeight;
			obj["hotspot_x"] = cursor.second.customHotspotX;
			obj["hotspot_y"] = cursor.second.customHotspotY;
			obj["preview"] = cursor.second.preview.bits;
			obj["preview_width"] = cursor.second.preview.width;
			obj["preview_height"] = cursor.second.preview.height;

			Settings["cursors"].push_back(obj);
		}

		Mutex.lock();
		{
			std::ofstream file(SettingsPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Mutex.unlock();
	}

	static void LoadCustomCursors()
	{
		for (auto& cursor : cursors)
		{
			LoadCustomCursor(cursor);
		}
	}

} // namespace Settings
