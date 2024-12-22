#include "cursor_hash.h"
#include "cursor_load.h"
#include "cursor_preview.h"
#include "settings.h"
#include "shared.h"
#include "utilities.h"
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

static bool isSetProcessPointers = false;

static void GetProcessPointers();
static void GetCursorPreview(HCURSOR hCursor, CursorProperties& properties);

/*******************************************************************************
 * HOOK :: SetCursor
 ******************************************************************************/
LPVOID pSetCursor = nullptr; /* address of SetCursor */
typedef HCURSOR(WINAPI *SETCURSOR)(HCURSOR); /* define calling convention */
SETCURSOR fpSetCursor = NULL; /* pointer to call original SetCursor */
HCURSOR WINAPI DetourSetCursor(HCURSOR hCursor)
{
    uint32_t key = GetCursorHash(hCursor);
    auto& io = ImGui::GetIO();

    if (MumbleLink->Context.IsInCombat && CombatCursor.second.customCursor)
    {
        hCustomCursor = CombatCursor.second.customCursor;
    }
    else if (io.WantCaptureMouse && NexusCursor.second.customCursor)
    {
        hCustomCursor = NexusCursor.second.customCursor;
    }
    else if (key != HASH_INVALID)
    {
        auto it = Cursors.find(key);
        if (it != Cursors.end())
        {
            /* set cursor to custom */
            hCustomCursor = it->second.customCursor;

            if (it->second.defaultPreview.bits.empty() || !it->second.defaultPreview.resource)
            {
                /* get preview for default cursor */
                GetCursorPreview(hCursor, it->second);
            }
        }
        else
        {
            /* key does not exist */
            Cursors.insert(CursorPair(key, CursorProperties()));
            GetCursorPreview(hCursor, Cursors[key]);
        }
    }
    else
    {
        /* set cursor to default */
        hCustomCursor = hCursor;
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
    
    DXGI_SWAP_CHAIN_DESC desc{};
    ((IDXGISwapChain*)APIDefs->SwapChain)->GetDesc(&desc);
    hClient = desc.OutputWindow;

    GetProcessPointers();

    APIDefs->Renderer.Register(ERenderType_PreRender, AddonRenderPreview);
    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

    GameDir = APIDefs->Paths.GetGameDirectory();
    AddonDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/");
    CacheDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/appcache/");
    IconsDir = APIDefs->Paths.GetAddonDirectory("CustomCursors/cursors/");

    std::filesystem::create_directory(AddonDir);
    std::filesystem::create_directory(CacheDir);
    std::filesystem::create_directory(IconsDir);

    Settings::LoadSettings(APIDefs->Paths.GetAddonDirectory("CustomCursors/settings.json"));
    Settings::LoadPreviews(APIDefs->Paths.GetAddonDirectory("CustomCursors/appcache/previews.json"));
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
        if (preview != nullptr)
        {
            CreateResourceFromBits(preview->width, preview->height, preview->bits, &(preview->resource));
        }
        aQueuedPreview.erase(aQueuedPreview.begin());
    }
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

void RowHeader(std::string c1, std::string c2, std::string c3, std::string c4, std::string c5, std::string c6);
void RowCursor(std::string identifier, CursorPair& cursor, float32_t& inputWidth);
void ColumnPreview(CursorPair& cursor);
void ColumnIdentifier(std::string identifier);
void ColumnFilepath(float32_t& inputWidth, CursorPair& cursor);
void ColumnWidth(float32_t& inputWidth, CursorPair& cursor);
void ColumnHeight(float32_t& inputWidth, CursorPair& cursor);
void ColumnHotspotX(float32_t& inputWidth, CursorPair& cursor);
void ColumnHotspotY(float32_t& inputWidth, CursorPair& cursor);
void ColumnRemove(CursorPair& cursor);

void AddonOptions()
{
    static const float32_t windowPadding = 24;
    float32_t inputWidth = (ImGui::GetWindowContentRegionWidth() - windowPadding) / 7;
    
    ImGui::BeginTable("Cursors", 8, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit);
    {
        RowHeader("UID", "Filepath", "Width", "Height", "Hotspot X", "Hotspot Y");
        RowCursor("Combat", CombatCursor, inputWidth);
        RowCursor("Nexus", NexusCursor, inputWidth);
        for (auto& cursor : Cursors)
        {
            RowCursor(std::to_string(cursor.first), cursor, inputWidth);
        }

        ImGui::EndTable();
    }
}

void RowHeader(std::string c1, std::string c2, std::string c3, std::string c4, std::string c5, std::string c6)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(1); ImGui::Text(c1.c_str()); /* Identifier */
    ImGui::TableSetColumnIndex(2); ImGui::Text(c2.c_str()); /* Filepath */
    ImGui::TableSetColumnIndex(3); ImGui::Text(c3.c_str()); /* Width */
    ImGui::TableSetColumnIndex(4); ImGui::Text(c4.c_str()); /* Height */
    ImGui::TableSetColumnIndex(5); ImGui::Text(c5.c_str()); /* Hotspot X */
    ImGui::TableSetColumnIndex(6); ImGui::Text(c6.c_str()); /* Hotspot Y */
}

void RowCursor(std::string identifier, CursorPair& cursor, float32_t& inputWidth)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ColumnPreview(cursor);
    ImGui::TableSetColumnIndex(1); ColumnIdentifier(identifier);
    ImGui::TableSetColumnIndex(2); ColumnFilepath(inputWidth, cursor);
    if (cursor.second.customFilePath != "")
    {
        ImGui::TableSetColumnIndex(3); ColumnWidth(inputWidth, cursor);
        ImGui::TableSetColumnIndex(4); ColumnHeight(inputWidth, cursor);

        if (cursor.second.customFileFormat == E_FILE_FORMAT_PNG)
        {
            ImGui::TableSetColumnIndex(5); ColumnHotspotX(inputWidth, cursor);
            ImGui::TableSetColumnIndex(6); ColumnHotspotY(inputWidth, cursor);
        }

        ImGui::TableSetColumnIndex(7); ColumnRemove(cursor);
    }
}

void ColumnPreview(CursorPair& cursor)
{
    auto resource = cursor.second.customPreview.resource ? cursor.second.customPreview.resource : cursor.second.defaultPreview.resource;

    if (resource != nullptr)
    {
        ImGui::Image((ImTextureID)(intptr_t)resource, ImVec2(32, 32));

        if (ImGui::IsItemHovered())
        {
            if (ImGui::Tooltip())
            {
                ImGui::BeginTable("PreviewTooltip", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInner);
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("Default");

                    ImGui::TableNextColumn();
                    ImGui::Text("Custom");

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (cursor.second.defaultPreview.resource != nullptr)
                    {
                        ImGui::Image(cursor.second.defaultPreview.resource, ImVec2(cursor.second.defaultPreview.width, cursor.second.defaultPreview.height));
                    }

                    ImGui::TableNextColumn();
                    if (cursor.second.customPreview.resource != nullptr)
                    {
                        ImGui::Image(cursor.second.customPreview.resource, ImVec2(cursor.second.customPreview.width, cursor.second.customPreview.height));
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTooltip();
            }
        }
    }
}

void ColumnIdentifier(std::string identifier)
{
    static const float textSize = ImGui::CalcTextSize("WWWWWWWWWW").x;

    ImGui::PushItemWidth(textSize);
    ImGui::PaddedText(identifier.c_str(), 0.0F, 2.0F);
    ImGui::PopItemWidth();
}

void ColumnFilepath(float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
    ImGui::PushItemWidth(inputWidth * 2);
    if (ImGui::Button((cursor.second.customFilePath + "##File-" + std::to_string(cursor.first)).c_str(), ImVec2(ImGui::CalcItemWidth(), 0)))
    {
        std::thread([&cursor] {
            OPENFILENAME ofn{};
            TCHAR szFile[MAX_PATH]{};
            TCHAR initialDir[MAX_PATH]{};

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
                cursor.second = CursorProperties();
                cursor.second.customFilePath = string_utils::replace_substr(std::string(ofn.lpstrFile), (GameDir.string() + "\\"), "");
                LoadCustomCursor(cursor.second);
                Settings::Save();
            }
        }).detach();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
}

void ColumnWidth(float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("##Width" + std::to_string(cursor.first)).c_str(), &(cursor.second.customWidth), 8U, 8U))
    {
        if (cursor.second.customWidth < 1)
        {
            cursor.second.customWidth = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
}

void ColumnHeight(float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("##Height" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHeight), 8U, 8U))
    {
        if (cursor.second.customHeight < 1)
        {
            cursor.second.customHeight = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
}

void ColumnHotspotX(float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("##HotspotX" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotX), 4U, 4U))
    {
        if (cursor.second.customHotspotX < 1)
        {
            cursor.second.customHotspotX = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
}

void ColumnHotspotY(float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0, 2.0));
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("##HotspotY" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotY), 4U, 4U))
    {
        if (cursor.second.customHotspotY < 1)
        {
            cursor.second.customHotspotY = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
}

void ColumnRemove(CursorPair& cursor)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0, 2.0));
    if (ImGui::Button(("x##Remove" + std::to_string(cursor.first)).c_str()))
    {
        cursor.second = CursorProperties();
        Settings::Save();
    }
    ImGui::PopStyleVar();
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

static void GetCursorPreview(HCURSOR hCursor, CursorProperties& properties)
{
    if (GetBitsFromCursor(hCursor, properties.defaultPreview.width, properties.defaultPreview.height, properties.defaultPreview.bits));
    {
        aQueuedPreview.push_back(&properties.defaultPreview);
    }
}
