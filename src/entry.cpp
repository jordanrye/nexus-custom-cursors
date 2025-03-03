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
void AddonQueueTexture();
void AddonQueueDelete();
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

    if (MumbleLink->Context.IsInCombat && CombatCursor.second.customCursor)
    {
        hCustomCursor = CombatCursor.second.customCursor;
    }
    else if (ImGui::GetIO().WantCaptureMouse && NexusCursor.second.customCursor)
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

    APIDefs->Renderer.Register(ERenderType_PreRender, AddonQueueTexture);
    APIDefs->Renderer.Register(ERenderType_PreRender, AddonQueueDelete);
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
    APIDefs->Renderer.Deregister(AddonQueueDelete);
    APIDefs->Renderer.Deregister(AddonQueueTexture);

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

void AddonQueueTexture()
{
    while (aQueueTexture.size() > 0)
    {
        auto preview = aQueueTexture.front();
        if (preview != nullptr)
        {
            CreateResourceFromBits(preview->width, preview->height, preview->bits, &(preview->resource));
        }
        aQueueTexture.pop();
    }

    if ((nullptr == NexusIcon) || (nullptr == NexusIcon->Resource))
    {
        NexusIcon = APIDefs->Textures.Get("ICON_NEXUS_HOVER");
    }

    if ((nullptr == CombatIcon) || (nullptr == CombatIcon->Resource))
    {
        CombatIcon = APIDefs->Textures.GetOrCreateFromResource("CC_ICON_COMBAT", CC_ICON_COMBAT, hSelf);
    }
}

void AddonQueueDelete()
{
    while (aQueueDelete.size() > 0)
    {
        Cursors.erase(aQueueDelete.front());
        aQueueDelete.pop();
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

static void ItemSelectableImage(Hash UID, ID3D11ShaderResourceView* icon, const ImVec2& iconSize, int& selected, const ImVec2& selectableSize);
static void ItemSelectableText(const char* str, int& selected, const ImVec2& selectableSize, const ImVec2& textSize);
static void ItemOptions(CursorPair& cursor, int& selected, const float32_t& inputWidth);
static void ItemOptionsGeneral(int& selected, const float32_t& inputWidth);
static void ColumnIdentifier(const char* identifier);
static void ColumnPreview(const float32_t& inputWidth, CursorPreview& preview);
static void ColumnFilepath(const float32_t& inputWidth, CursorPair& cursor);
static void ColumnWidth(const float32_t& inputWidth, CursorPair& cursor);
static void ColumnHeight(const float32_t& inputWidth, CursorPair& cursor);
static void ColumnHotspotX(const float32_t& inputWidth, CursorPair& cursor);
static void ColumnHotspotY(const float32_t& inputWidth, CursorPair& cursor);
static void ColumnRemove(CursorPair& cursor);

void AddonOptions()
{
    static int selected = 0;

    /* navigation view */
    {
        static const ImVec2 minPadding(10.f, 10.f);
        static const ImVec2 iconSize(32.f, 32.f);
        static const char* text = "General";
        static const ImVec2 textSize = ImGui::CalcTextSize(text);
        static const ImVec2 _maxSize = ImGui::MaxSizeImVec2(iconSize, textSize);
        static const ImVec2 selectableTextSize = {
            _maxSize.x + minPadding.x + ImGui::GetStyle().ScrollbarSize,
            textSize.y + minPadding.y
        };
        static const ImVec2 selectableImageSize = {
            _maxSize.x + minPadding.x + ImGui::GetStyle().ScrollbarSize,
            iconSize.y
        };
        static const ImVec2 _maxSelectableSize = ImGui::MaxSizeImVec2(selectableTextSize, selectableImageSize);

        if (ImGui::BeginChild("##Navigation", ImVec2(_maxSelectableSize.x, 0.f), true, ImGuiWindowFlags_NoResize))
        {
            ItemSelectableText(text, selected, selectableTextSize, textSize);

            if ((nullptr != NexusIcon) && (nullptr != NexusIcon->Resource))
            {
                ItemSelectableImage(NexusCursor.first, (ID3D11ShaderResourceView*)(NexusIcon->Resource), iconSize, selected, selectableImageSize);
            }
            
            if ((nullptr != CombatIcon) && (nullptr != CombatIcon->Resource))
            {
                ItemSelectableImage(CombatCursor.first, (ID3D11ShaderResourceView*)(CombatIcon->Resource), iconSize, selected, selectableImageSize);
            }

            for (auto& cursor : Cursors)
            {
                ItemSelectableImage(cursor.first, cursor.second.defaultPreview.resource, iconSize, selected, selectableImageSize);
            }
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    /* options view */
    {
        const float32_t inputWidth = ImGui::GetWindowContentRegionWidth() * 0.65f;

        ImGui::BeginGroup();
        if (ImGui::BeginChild("##Content", ImVec2(-FLT_MIN, 0.0f), true, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ItemOptionsGeneral(selected, inputWidth);
            ItemOptions(NexusCursor, selected, inputWidth);
            ItemOptions(CombatCursor, selected, inputWidth);

            for (auto& cursor : Cursors)
            {
                ItemOptions(cursor, selected, inputWidth);
            }
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }
}

static void ItemSelectableImage(Hash UID, ID3D11ShaderResourceView* icon, const ImVec2& iconSize, int& selected, const ImVec2& selectableSize)
{
    const ImVec2 pos = ImGui::GetCursorPos();
    const ImVec2 padding = {
        (selectableSize.x - iconSize.x - ImGui::GetStyle().ScrollbarSize) / 2,
        (selectableSize.y - iconSize.y) / 2
    };

    /* render selectable area */
    ImGui::SetCursorPos(pos);
    if (ImGui::Selectable(("##" + std::to_string(UID)).c_str(), (selected == UID), ImGuiSelectableFlags_None, selectableSize))
    {
        selected = UID;
    }

    /* render image */
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPos(ImVec2((pos.x + padding.x), (pos.y + padding.y)));
    ImGui::Image(icon, iconSize);
    ImGui::SetCursorPos(ImVec2(pos.x, (pos.y + selectableSize.y + ImGui::GetStyle().ItemSpacing.y)));
}

static void ItemSelectableText(const char* str, int& selected, const ImVec2& selectableSize, const ImVec2& textSize)
{
    const ImVec2 pos = ImGui::GetCursorPos();
    const ImVec2 padding = {
        (selectableSize.x - textSize.x - ImGui::GetStyle().ScrollbarSize) / 2,
        (selectableSize.y - textSize.y) / 2
    };

    /* render selectable area */
    ImGui::SetCursorPos(pos);
    if (ImGui::Selectable(("##" + std::string(str)).c_str(), (selected == 0), ImGuiSelectableFlags_None, selectableSize))
    {
        selected = 0;
    }

    /* render text */
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPos(ImVec2((pos.x + padding.x), (pos.y + padding.y)));
    ImGui::Text(str);
    ImGui::SetCursorPos(ImVec2(pos.x, (pos.y + selectableSize.y + ImGui::GetStyle().ItemSpacing.y)));
}

static void ItemOptions(CursorPair& cursor, int& selected, const float32_t& inputWidth)
{
    if (selected == cursor.first)
    {
        auto preview = cursor.second.customPreview.resource ? cursor.second.customPreview : cursor.second.defaultPreview;

        ColumnIdentifier(std::to_string(cursor.first).c_str());
        ColumnPreview(inputWidth, preview);
        ColumnFilepath(inputWidth, cursor);

        if (cursor.second.customFilePath != "")
        {
            ColumnWidth(inputWidth, cursor);
            ColumnHeight(inputWidth, cursor);

            if (cursor.second.customFileFormat == E_FILE_FORMAT_PNG)
            {
                ColumnHotspotX(inputWidth, cursor);
                ColumnHotspotY(inputWidth, cursor);
            }
        }
        
        ColumnRemove(cursor);
    }
}

static void ItemOptionsGeneral(int& selected, const float32_t& inputWidth)
{
    static bool _flag1 = false;
    static bool _flag2 = false;
    static bool _flag3 = false;
    static bool _flag4 = false;

    if (selected == 0)
    {
        ImGui::BeginGroupPanel("Special Cursors", ImVec2(inputWidth, 0.f));
        if (ImGui::Checkbox("Add 'in-combat' cursor override", &_flag1))
        {
            /* do something */
        }
        if (ImGui::Checkbox("Add 'Nexus' cursor override", &_flag2))
        {
            /* do something */
        }
        ImGui::EndGroupPanel();

        ImGui::BeginGroupPanel("Configuration", ImVec2(inputWidth, 0.f));
        if (ImGui::Checkbox("Separate width and height inputs", &_flag3))
        {
            /* do something */
        }
        ImGui::EndGroupPanel();
        
        ImGui::BeginGroupPanel("Debugging", ImVec2(inputWidth, 0.f));
        if (ImGui::Checkbox("Enable debug window", &_flag4))
        {
            /* do something */
        }
        ImGui::EndGroupPanel();
    }
}

static void ColumnIdentifier(const char* identifier)
{
    ImGui::Text("UID:");
    ImGui::SameLine();
    ImGui::Text(identifier);
}

static void ColumnPreview(const float32_t& inputWidth, CursorPreview& preview)
{
    ImGui::BeginGroupPanel("Preview", ImVec2(inputWidth, 0.f));
    auto pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2((pos.x + ((inputWidth - preview.width) / 2)), pos.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 20.f));
    if (nullptr != preview.resource)
    {
        ImGui::Image((ImTextureID)(intptr_t)preview.resource, ImVec2(preview.width, preview.height));
    }
    ImGui::PopStyleVar(1);
    ImGui::EndGroupPanel();
}

static void ColumnFilepath(const float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::Button((cursor.second.customFilePath + "##" + std::to_string(cursor.first)).c_str(), ImVec2(ImGui::CalcItemWidth(), 0)))
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
                auto defaultPreview = cursor.second.defaultPreview;
                
                cursor.second = CursorProperties();
                cursor.second.defaultPreview = defaultPreview;
                cursor.second.customFilePath = string_utils::replace_substr(std::string(ofn.lpstrFile), (GameDir.string() + "\\"), "");
                LoadCustomCursor(cursor.second);
                Settings::Save();
            }
        }).detach();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::TextPadded("Filepath", ImVec2(0.f, ImGui::GetStyle().ItemSpacing.y));
}

static void ColumnWidth(const float32_t& inputWidth, CursorPair& cursor)
{
    const auto originalWidth = static_cast<float32_t>(cursor.second.customWidth);
    const auto originalHeight = static_cast<float32_t>(cursor.second.customHeight);

    ImGui::PushItemWidth(inputWidth);
    std::string format = std::to_string(cursor.second.customWidth) + "x" + std::to_string(cursor.second.customHeight);
    if (ImGui::SliderIntStep(("Size##" + std::to_string(cursor.first)).c_str(), &(cursor.second.customWidth), 8U, 256U, 8U, format.c_str(), ImGuiSliderFlags_NoInput))
    {
        if (cursor.second.customWidth < 1)
        {
            cursor.second.customWidth = 1;
        }

        cursor.second.customHeight = static_cast<int32_t>(originalHeight * (static_cast<float32_t>(cursor.second.customWidth) / originalWidth));

        if (cursor.second.customHeight < 1)
        {
            cursor.second.customHeight = 1;
        } 

        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();

    // ImGui::PushItemWidth(inputWidth);
    // if (ImGui::InputInt(("Width##" + std::to_string(cursor.first)).c_str(), &(cursor.second.customWidth), 8U, 8U))
    // {
    //     if (cursor.second.customWidth < 1)
    //     {
    //         cursor.second.customWidth = 1;
    //     }
    //     LoadCustomCursor(cursor.second);
    //     Settings::Save();
    // }
    // ImGui::PopItemWidth();
}

static void ColumnHeight(const float32_t& inputWidth, CursorPair& cursor)
{
    // ImGui::PushItemWidth(inputWidth);
    // if (ImGui::InputInt(("Height##" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHeight), 8U, 8U))
    // {
    //     if (cursor.second.customHeight < 1)
    //     {
    //         cursor.second.customHeight = 1;
    //     }
    //     LoadCustomCursor(cursor.second);
    //     Settings::Save();
    // }
    // ImGui::PopItemWidth();
}

static void ColumnHotspotX(const float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("Hotspot X##" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotX), 4U, 4U))
    {
        if (cursor.second.customHotspotX < 1)
        {
            cursor.second.customHotspotX = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
}

static void ColumnHotspotY(const float32_t& inputWidth, CursorPair& cursor)
{
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::InputInt(("Hotspot Y##" + std::to_string(cursor.first)).c_str(), &(cursor.second.customHotspotY), 4U, 4U))
    {
        if (cursor.second.customHotspotY < 1)
        {
            cursor.second.customHotspotY = 1;
        }
        LoadCustomCursor(cursor.second);
        Settings::Save();
    }
    ImGui::PopItemWidth();
}

static void ColumnRemove(CursorPair& cursor)
{
    if (ImGui::Button(("Delete##" + std::to_string(cursor.first)).c_str()))
    {
        cursor.second = CursorProperties();
        aQueueDelete.push(cursor.first);
        Settings::Save();
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

static void GetCursorPreview(HCURSOR hCursor, CursorProperties& properties)
{
    if (GetBitsFromCursor(hCursor, properties.defaultPreview.width, properties.defaultPreview.height, properties.defaultPreview.bits));
    {
        aQueueTexture.push(&properties.defaultPreview);
    }
}
