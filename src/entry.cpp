#include "cursor_hash.h"
#include "cursor_load.h"
#include "cursor_preview.h"
#include "settings.h"
#include "shared.h"
#include "version.h"

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

#include <Windows.h>
#include <filesystem>
#include <sstream>
#include <string>

void OnMumbleIdentityUpdated(void* aEventArgs);
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRenderPreview();
void AddonRender();
void AddonOptions();

static void GetProcessPointers();
static bool isSetProcessPointers = false;

/// FIXME: temporary hack
static Hash _UID = 0;
static bool _ConfigModeEnabled = false;

/*******************************************************************************
 * HOOK :: SetCursor
 ******************************************************************************/
LPVOID pSetCursor = nullptr; /* address of SetCursor */
typedef HCURSOR(WINAPI *SETCURSOR)(HCURSOR); /* define calling convention */
SETCURSOR fpSetCursor = NULL; /* pointer to call original SetCursor */
HCURSOR WINAPI DetourSetCursor(HCURSOR hCursor)
{
    uint32_t key = GetCursorHash(hCursor);

    if (key != HASH_INVALID)
    {
        auto it = Cursors.find(key);

        if (it != Cursors.end())
        {
            /* update cursor */
            hCustomCursor = it->second.customCursor;

            if (it->second.preview.bits.empty())
            {
                /* get bits for default cursor */
                GetBitsFromCursor(hCursor, it->second.preview.width, it->second.preview.height, it->second.preview.bits);
            }
            if (it->second.preview.resource == nullptr)
            {
                /* queue bits for resource creation */
                aQueuedPreview.push_back(&it->second.preview);
            }

            _UID = key;	/** FIXME: temporary hack */
        }
        else
        {
            /* key does not exist */
            CursorProperties properties{};
            GetBitsFromCursor(hCursor, properties.preview.width, properties.preview.height, properties.preview.bits);
            aQueuedPreview.push_back(&properties.preview);
            Cursors[key] = properties;
        }
    }

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
typedef HCURSOR(WINAPI *SETCLASSLONGPTRA)(HWND, INT, LONG_PTR); /* define calling convention */
SETCLASSLONGPTRA fpSetClassLongPtrA = NULL; /* pointer to call original SetClassLongPtrA */
HCURSOR WINAPI DetourSetClassLongPtrA(HWND hWnd, INT nIndex, LONG_PTR dwNewLong)
{
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
typedef HCURSOR(WINAPI* SETCLASSLONGPTRW)(HWND, INT, LONG_PTR); /* define calling convention */
SETCLASSLONGPTRW fpSetClassLongPtrW = NULL; /* pointer to call original SetClassLongPtrW */
HCURSOR WINAPI DetourSetClassLongPtrW(HWND hWnd, INT nIndex, LONG_PTR dwNewLong)
{
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
    AddonDef.Description = "Replace in-game cursors with custom icons.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_IsVolatile;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/jordanrye/nexus-custom-cursors-public";

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

    ((IDXGISwapChain*)APIDefs->SwapChain)->GetDevice(__uuidof(ID3D11Device), (void**)&D3D11Device);

    GetProcessPointers();

    APIDefs->Renderer.Register(ERenderType_PreRender, AddonRenderPreview);
    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

    AddonDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/");
    IconsDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/cursors/");

    std::filesystem::create_directory(AddonDir);
    std::filesystem::create_directory(IconsDir);

    /* load cursors and queue previews for render */
    Settings::LoadSettings(APIDefs->Paths.GetAddonDirectory("CustomCursors/settings.json"));
    Settings::LoadPreviews(APIDefs->Paths.GetAddonDirectory("CustomCursors/previews.json"));
    for (auto& cursor : Cursors)
    {
        aQueuedPreview.push_back(&cursor.second.preview);
    }
}

void AddonUnload()
{
    APIDefs->Renderer.Deregister(AddonOptions); 
    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonRenderPreview);

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

    D3D11Device = nullptr;

    APIDefs->Events.Unsubscribe("EV_MUMBLE_IDENTITY_UPDATED", OnMumbleIdentityUpdated);

    MumbleLink = nullptr;
    NexusLink = nullptr;

    APIDefs = nullptr;

    /* save configuration */
    Settings::Save();
}

void AddonRenderPreview()
{
    while (aQueuedPreview.size() > 0)
    {
        auto preview = aQueuedPreview.front();
        CreateResourceFromBits(preview->width, preview->height, preview->bits, &(preview->resource));
        aQueuedPreview.erase(aQueuedPreview.begin());
    }
}

void AddonRender()
{
    static bool isHookedSetCursor = false;
    static bool isHookedSetClassLongPtrA = false;
    static bool isHookedSetClassLongPtrW = false;
    
    /// FIXME: temporary hack
    static bool isHooked = false;

    // render configuration mode tooltip
    if (_ConfigModeEnabled)
    {
        if (ImGui::Begin("Timeout", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs))
        {
            POINT CursorPos;
            GetCursorPos(&CursorPos);
            ImGui::SetWindowPos(ImVec2(static_cast<float32_t>(CursorPos.x + 42), static_cast<float32_t>(CursorPos.y)));
            std::stringstream ss;
            ss << "UID: " << std::to_string(_UID);
            ImGui::Text(ss.str().c_str());
        }
        ImGui::End();
    }

    if (!isHooked && isSetProcessPointers)
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
            /// FIXME: temporary hack
            /// TODO: split into separate Renderer functions
            isHooked = true;
            // std::thread([]() {
            // 	APIDefs->Renderer.Deregister(AddonRender);
            // }).detach();
        }
    }
}

void AddonOptions()
{
    ImGui::Checkbox("Configuration Mode", &_ConfigModeEnabled);

    ImGui::BeginTable("Cursors", 8, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit);
    {
        static const float windowPadding = 24;
        float inputWidth = (ImGui::GetWindowContentRegionWidth() - windowPadding) / 9;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("UID");
        ImGui::TableSetColumnIndex(1); ImGui::Text("Filepath");
        ImGui::TableSetColumnIndex(2); ImGui::Text("Width");
        ImGui::TableSetColumnIndex(3); ImGui::Text("Height");
        ImGui::TableSetColumnIndex(4); ImGui::Text("Hotspot X");
        ImGui::TableSetColumnIndex(5); ImGui::Text("Hotspot Y");

        for (auto& cursor : Cursors)
        {
            ImGui::TableNextRow();

            //("Cursor##" + std::to_string(cursor.first)).c_str()
        
            ImGui::TableSetColumnIndex(0);
            ImGui::PaddedText(std::to_string(cursor.first).c_str(), 0.0F, 2.0F);

            ImGui::TableSetColumnIndex(1);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
            ImGui::PushItemWidth(inputWidth * 2);
            if (ImGui::Button((cursor.second.customFilePath + "##File-" + std::to_string(cursor.first)).c_str(), ImVec2(ImGui::CalcItemWidth(), 0)))
            {
                std::thread([&cursor]{
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
                        cursor.second.customFilePath = std::string(ofn.lpstrFile).substr(IconsDir.string().length());
                        LoadCustomCursor(cursor);
                        Settings::Save();
                    }
                }).detach();
            }
            ImGui::PopItemWidth();
            ImGui::PopStyleVar();

            if (cursor.second.customFilePath != "")
            {
                ImGui::TableSetColumnIndex(2);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
                ImGui::PushItemWidth(inputWidth);
                if (ImGui::InputInt(("##Width" + std::to_string(cursor.first)).c_str(), &(cursor.second.customWidth), 8U, 8U))
                {
                    LoadCustomCursor(cursor);
                    Settings::Save();
                }
                ImGui::PopItemWidth();
                ImGui::PopStyleVar();

                ImGui::TableSetColumnIndex(3);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
                ImGui::PushItemWidth(inputWidth);
                if (ImGui::InputInt(("##Height" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHeight), 8U, 8U))
                {
                    LoadCustomCursor(cursor);
                    Settings::Save();
                }
                ImGui::PopItemWidth();
                ImGui::PopStyleVar();

                if (cursor.second.customFileFormat == E_FILE_FORMAT_PNG)
                {
                    ImGui::TableSetColumnIndex(4);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
                    ImGui::PushItemWidth(inputWidth);
                    if (ImGui::InputInt(("##HotspotX" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotX), 4U, 4U))
                    {
                        LoadCustomCursor(cursor);
                        Settings::Save();
                    }
                    ImGui::PopItemWidth();
                    ImGui::PopStyleVar();

                    ImGui::TableSetColumnIndex(5);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
                    ImGui::PushItemWidth(inputWidth);
                    if (ImGui::InputInt(("##HotspotY" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotY), 4U, 4U))
                    {
                        LoadCustomCursor(cursor);
                        Settings::Save();
                    }
                    ImGui::PopItemWidth();
                    ImGui::PopStyleVar();
                }

                ImGui::TableSetColumnIndex(6);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0, 2.0));
                if (ImGui::Button(("x##Remove" + std::to_string(cursor.first)).c_str()))
                {
                    cursor.second = CursorProperties();
                    Settings::Save();
                }
                ImGui::PopStyleVar();
            }

            ImGui::TableSetColumnIndex(7);
            auto resource = cursor.second.preview.resource;
            if (resource != nullptr)
            {
                ImGui::Image((ImTextureID)(intptr_t)resource, ImVec2(32, 32));
            }
        }

        ImGui::EndTable();
    }
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
