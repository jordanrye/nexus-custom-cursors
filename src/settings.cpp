#include <filesystem>
#include <fstream>

#include "create_cursor.h"

#include "settings.h"

namespace Settings
{
	static void SetCustomCursor();
	static bool IsFileType(const std::string& str, const std::string& ext);

	std::mutex Mutex;
	std::filesystem::path SettingsPath;
	json Settings = json::object();

	/* Cursor configuration */
	std::string CursorFilePath = "(null)";
	E_FILE_FORMAT CursorFileFormat = E_FILE_FORMAT_INVALID;
	INT CursorWidth = 0;
	INT CursorHeight = 0;
	INT CursorHotspotX = 0;
	INT CursorHotspotY = 0;

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
			if (!Settings["CURSOR_FILE_PATH"].is_null()) { Settings["CURSOR_FILE_PATH"].get_to(CursorFilePath); }
			if (!Settings["CURSOR_FILE_FORMAT"].is_null()) { Settings["CURSOR_FILE_FORMAT"].get_to(CursorFileFormat); }
			if (!Settings["CURSOR_WIDTH"].is_null()) { Settings["CURSOR_WIDTH"].get_to(CursorWidth); }
			if (!Settings["CURSOR_HEIGHT"].is_null()) { Settings["CURSOR_HEIGHT"].get_to(CursorHeight); }
			if (!Settings["CURSOR_HOTSPOT_X"].is_null()) { Settings["CURSOR_HOTSPOT_X"].get_to(CursorHotspotX); }
			if (!Settings["CURSOR_HOTSPOT_Y"].is_null()) { Settings["CURSOR_HOTSPOT_Y"].get_to(CursorHotspotY); }
		}

		SetCustomCursor();
	}

	void Save()
	{
		SetCustomCursor();

		Settings["CURSOR_FILE_PATH"] = CursorFilePath;
		Settings["CURSOR_FILE_FORMAT"] = CursorFileFormat;
		Settings["CURSOR_WIDTH"] = CursorWidth;
		Settings["CURSOR_HEIGHT"] = CursorHeight;
		Settings["CURSOR_HOTSPOT_X"] = CursorHotspotX;
		Settings["CURSOR_HOTSPOT_Y"] = CursorHotspotY;

		Mutex.lock();
		{
			std::ofstream file(SettingsPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Mutex.unlock();
	}

	static void SetCustomCursor()
	{
		std::string filename = Gw2RootDir.string() + CursorFilePath;

		if (IsFileType(filename, ".png"))
		{
			hCustomCursor = CreateCursorFromPNG(filename, CursorWidth, CursorHeight, CursorHotspotX, CursorHotspotY);
		}
		else if (IsFileType(filename, ".cur") || IsFileType(filename, ".ani"))
		{
			HANDLE hImage = LoadImage(NULL, filename.c_str(), IMAGE_CURSOR, CursorWidth, CursorHeight, LR_LOADFROMFILE);
			hCustomCursor = static_cast<HCURSOR>(hImage);
		}
		else
		{
			hCustomCursor = NULL;
		}
	}

	static bool IsFileType(const std::string& str, const std::string& ext)
	{
		return str.compare(str.size() - ext.size(), ext.size(), ext) == 0;
	}

} // namespace Settings
