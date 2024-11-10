#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

#include "imgui/imgui.h"

namespace string_utils 
{
    static wchar_t* cstr_to_wcstr(const char* c)
    {
        const size_t cSize = strlen(c);
        const size_t wcSize = cSize + 1;
        wchar_t* wc = new wchar_t[wcSize];
        mbstowcs_s(NULL, wc, wcSize, c, cSize);
        return wc;
    }

    static std::string wcstr_to_string(const wchar_t* wc)
    {
        std::wstring ws(wc);
        std::string s(ws.begin(), ws.end());
        return s;
    }

    static const wchar_t* string_to_wcstr(std::string s)
    {
        std::wstring ws = std::wstring(s.begin(), s.end());
        const wchar_t* wc = ws.c_str();
        return wc;
    }

    static std::string replace_substr(std::string str, std::string oldSubStr, std::string newSubStr, size_t pos = 0)
    {
        while ((pos = str.find(oldSubStr, pos)) != std::string::npos) {
            str.replace(pos, oldSubStr.size(), newSubStr);
            pos += newSubStr.size();
        }
        return str;
    }

} // namespace string_utils

namespace ImGui
{
    static bool Tooltip()
    {
        bool hovered = ImGui::IsItemHovered();
        if (hovered)
        {
            ImGui::BeginTooltip();
        }
        return hovered;
    }

    static void TooltipGeneric(const char* str, ...)
    {
        if (ImGui::Tooltip())
        {
            ImGui::Text(str);
            ImGui::EndTooltip();
        }
    }

    static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
    {
        return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
    }

    static void PaddedText(const char* str, float paddingX, float paddingY)
    {
        ImVec2 textSize = ImGui::CalcTextSize(str);
        ImVec2 cursorStart = ImGui::GetCursorPos();
        ImGui::InvisibleButton("##PaddedText", textSize + ImVec2(paddingX * 2, paddingY * 2));
        ImVec2 cursorFinal = ImGui::GetCursorPos();
        ImGui::SetCursorPos(cursorStart + ImVec2(paddingX, paddingY));
        ImGui::Text(str);
        //ImGui::SetCursorPos(cursorFinal);
    }

} // namespace ImGui

#endif /* UTILITIES_H */
