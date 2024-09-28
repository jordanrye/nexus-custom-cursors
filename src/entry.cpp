#include <Windows.h>
#include <filesystem>

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

#include "settings.h"
#include "shared.h"
#include "version.h"

void OnMumbleIdentityUpdated(void* aEventArgs);
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

static void GetProcessPointers();
static bool isSetProcessPointers = false;

/*******************************************************************************
 * HOOK :: SetCursor
 ******************************************************************************/
LPVOID pSetCursor = nullptr; /* address of SetCursor */
typedef HCURSOR(WINAPI *SETCURSOR)(HCURSOR); /* define calling convention */
SETCURSOR fpSetCursor = NULL; /* pointer to call original SetCursor */
HCURSOR WINAPI DetourSetCursor(HCURSOR hCursor) {
	if (hCustomCursor != NULL)
	{
		return fpSetCursor(hCustomCursor);
	}
	else
	{
		return fpSetCursor(hCursor);
	}
}

/*******************************************************************************
 * HOOK :: SetClassLongPtrA
 ******************************************************************************/
LPVOID pSetClassLongPtrA = nullptr; /* address of SetClassLongPtrA */
typedef HCURSOR(WINAPI *SETCLASSLONGPTRA)(HWND, int, LONG_PTR); /* define calling convention */
SETCLASSLONGPTRA fpSetClassLongPtrA = NULL; /* pointer to call original SetClassLongPtrA */
HCURSOR WINAPI DetourSetClassLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
	if (GCLP_HCURSOR == nIndex)
	{
		return fpSetClassLongPtrA(hWnd, nIndex, (LONG_PTR)hCustomCursor);
	}
	return fpSetClassLongPtrA(hWnd, nIndex, dwNewLong);
}

/*******************************************************************************
 * HOOK :: SetClassLongPtrW
 ******************************************************************************/
LPVOID pSetClassLongPtrW = nullptr; /* address of SetClassLongPtrW */
typedef HCURSOR(WINAPI* SETCLASSLONGPTRW)(HWND, int, LONG_PTR); /* define calling convention */
SETCLASSLONGPTRW fpSetClassLongPtrW = NULL; /* pointer to call original SetClassLongPtrW */
HCURSOR WINAPI DetourSetClassLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
	if (GCLP_HCURSOR == nIndex)
	{
		return fpSetClassLongPtrW(hWnd, nIndex, (LONG_PTR)hCustomCursor);
	}
	return fpSetClassLongPtrW(hWnd, nIndex, dwNewLong);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hSelf = hModule;
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = 38;
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Custom Cursors";
	AddonDef.Version.Major = V_MAJOR;
	AddonDef.Version.Minor = V_MINOR;
	AddonDef.Version.Build = V_BUILD;
	AddonDef.Version.Revision = V_REVISION;
	AddonDef.Author = "Jordan";
	AddonDef.Description = "Replace the default mouse cursors with custom icons.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_IsVolatile;
	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/jordanrye/nexus-custom-cursors";

	return &AddonDef;
}

void OnMumbleIdentityUpdated(void* aEventArgs)
{
	MumbleIdentity = (Mumble::Identity*)aEventArgs;
}

void AddonLoad(AddonAPI* aApi)
{
	APIDefs = aApi;
	ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

	NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
	MumbleLink = (Mumble::Data*)APIDefs->DataLink.Get("DL_MUMBLE_LINK");

	APIDefs->Events.Subscribe("EV_MUMBLE_IDENTITY_UPDATED", OnMumbleIdentityUpdated);

	GetProcessPointers();

	APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
	APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

	Gw2RootDir = APIDefs->Paths.GetGameDirectory();
	AddonDir = APIDefs->Paths.GetAddonDirectory("CustomCursors");
	IconsDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/cursors");

	std::filesystem::create_directory(AddonDir);
	std::filesystem::create_directory(IconsDir);
	Settings::Load(APIDefs->Paths.GetAddonDirectory("CustomCursors/settings.json"));
}

void AddonUnload()
{
	APIDefs->Renderer.Deregister(AddonOptions); 
	APIDefs->Renderer.Deregister(AddonRender);

	if (pSetCursor)
	{
		APIDefs->MinHook.Disable(pSetCursor);
		APIDefs->MinHook.Remove(pSetCursor);
	}
	if (pSetClassLongPtrA)
	{
		APIDefs->MinHook.Disable(pSetClassLongPtrA);
		APIDefs->MinHook.Remove(pSetClassLongPtrA);
	}
	if (pSetClassLongPtrW)
	{
		APIDefs->MinHook.Disable(pSetClassLongPtrW);
		APIDefs->MinHook.Remove(pSetClassLongPtrW);
	}

	APIDefs->Events.Unsubscribe("EV_MUMBLE_IDENTITY_UPDATED", OnMumbleIdentityUpdated);

	MumbleLink = nullptr;
	NexusLink = nullptr;

	APIDefs = nullptr;

	Settings::Save();
}

void AddonRender()
{
	static bool isHookedSetCursor = false;
	static bool isHookedSetClassLongPtrA = false;
	static bool isHookedSetClassLongPtrW = false;

	if (isSetProcessPointers)
	{
		if (!isHookedSetCursor)
		{
			if (APIDefs->MinHook.Create(pSetCursor, &DetourSetCursor, reinterpret_cast<LPVOID*>(&fpSetCursor)) == MH_OK)
			{
				if (APIDefs->MinHook.Enable(pSetCursor) == MH_OK)
				{
					isHookedSetCursor = true;
					APIDefs->Log(ELogLevel_INFO, "CustomCursors", "Deployed hook for SetCursor.");
				}
			}
		}
		if (!isHookedSetClassLongPtrA)
		{
			if (APIDefs->MinHook.Create(pSetClassLongPtrA, &DetourSetClassLongPtrA, reinterpret_cast<LPVOID*>(&fpSetClassLongPtrA)) == MH_OK)
			{
				if (APIDefs->MinHook.Enable(pSetClassLongPtrA) == MH_OK)
				{
					isHookedSetClassLongPtrA = true;
					APIDefs->Log(ELogLevel_INFO, "CustomCursors", "Deployed hook for SetClassLongPtrA.");
				}
			}
		}
		if (!isHookedSetClassLongPtrW)
		{
			if (APIDefs->MinHook.Create(pSetClassLongPtrW, &DetourSetClassLongPtrW, reinterpret_cast<LPVOID*>(&fpSetClassLongPtrW)) == MH_OK)
			{
				if (APIDefs->MinHook.Enable(pSetClassLongPtrW) == MH_OK)
				{
					isHookedSetClassLongPtrW = true;
					APIDefs->Log(ELogLevel_INFO, "CustomCursors", "Deployed hook for SetClassLongPtrW.");
				}
			}
		}
		if (isHookedSetCursor && isHookedSetClassLongPtrA && isHookedSetClassLongPtrW)
		{
			std::thread([]() {
				APIDefs->Renderer.Deregister(AddonRender);
			}).detach();
		}
	}
}

void AddonOptions()
{
	ImGui::BeginTable("Movement", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::PaddedText("Cursor File", 0.0F, 4.0F);
	ImGui::TableNextColumn();
	if (ImGui::Button((Settings::CursorFilePath + "##Cursor").c_str(), ImVec2(ImGui::CalcItemWidth(), 0)))
	{
		std::thread([]{
			OPENFILENAME ofn = { 0 };
			TCHAR szFile[MAX_PATH] = { 0 };
			TCHAR initialDir[MAX_PATH] = { 0 };

			strcpy_s(initialDir, IconsDir.string().c_str());

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = static_cast<HWND>(nullptr);
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "All Files (*.*)\0*.*\0Supported Files (*.png, *.cur, *.ani)\0*.png;*.cur;*.ani\0Portable Network Graphic (*.png)\0*.png\0Windows Cursor File (*.cur)\0*.cur\0Windows Animated Cursor File (*.ani)\0*.ani\0";
			ofn.nFilterIndex = 2;
			ofn.lpstrInitialDir = initialDir;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn) == TRUE)
			{
				Settings::CursorFilePath = std::string(ofn.lpstrFile).substr(Gw2RootDir.string().length());
				Settings::Save();
			}
		}).detach();
	}

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::PaddedText("Cursor Width", 0.0F, 4.0F);
	ImGui::TableNextColumn();
	if (ImGui::InputInt("##CursorWidth", &Settings::CursorWidth, 8U, 8U))
	{
		Settings::Save();
	}

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::PaddedText("Cursor Height", 0.0F, 4.0F);
	ImGui::TableNextColumn();
	if (ImGui::InputInt("##CursorHeight", &Settings::CursorHeight, 8U, 8U))
	{
		Settings::Save();
	}

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::PaddedText("Cursor Hotspot X", 0.0F, 4.0F);
	ImGui::TableNextColumn();
	if (ImGui::InputInt("##CursorHotspotX", &Settings::CursorHotspotX, 4U, 4U))
	{
		Settings::Save();
	}

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::PaddedText("Cursor Hotspot Y", 0.0F, 4.0F);
	ImGui::TableNextColumn();
	if (ImGui::InputInt("##CursorHotspotY", &Settings::CursorHotspotY, 4U, 4U))
	{
		Settings::Save();
	}

	ImGui::EndTable();
}

static void GetProcessPointers()
{
	do
	{
		if (!pSetCursor)
		{
			pSetCursor = &SetCursor;
		}

		if (!pSetClassLongPtrA)
		{
			pSetClassLongPtrA = &SetClassLongPtrA;
		}

		if (!pSetClassLongPtrW)
		{
			pSetClassLongPtrW = &SetClassLongPtrW;
		}

		if (pSetCursor && pSetClassLongPtrA && pSetClassLongPtrW)
		{
			isSetProcessPointers = true;
		}
	} while (!isSetProcessPointers);
}
