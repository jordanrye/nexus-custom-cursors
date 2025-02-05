#ifndef IMGUI_EXTENSIONS_H
#define IMGUI_EXTENSIONS_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace
{
    static void AddUnderLine(ImColor col_)
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
    }
}

namespace ImGui
{
    static ImVec2 MaxSizeImVec2(const ImVec2& a, const ImVec2& b)
    {
        ImVec2 result{};
        result.x = (a.x > b.x ? a.x : b.x);
        result.y = (a.y > b.y ? a.y : b.y);
        return result;
    }

    static bool SliderIntStep(const char* label, int* v, int v_min, int v_max, int v_step, const char* format = "%d", ImGuiSliderFlags flags = 0)
    {
        bool value_changed = false;

        /* map values from [v_min, v_max] to [0, N] */
        int v_i = (*v - v_min) / v_step;
        int N = (v_max - v_min) / v_step;
        value_changed = SliderInt(label, &v_i, 0, N, format, flags);

        /* re-map values in range [v_min, v_max] */
        *v = v_min + v_i * v_step;
        
        return value_changed;
    }

    static bool Tooltip()
    {
        bool hovered = ImGui::IsItemHovered();
        if (hovered)
        {
            ImGui::BeginTooltip();
        }
        return hovered;
    }

    static void TooltipGeneric(const char* fmt, ...)
    {
        if (ImGui::Tooltip())
        {
            va_list args;
            va_start(args, fmt);
            ImGui::TextV(fmt, args);
            va_end(args);
            ImGui::EndTooltip();
        }
    }

    static void TooltipHelp(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    static void TextPadded(const char* str, ImVec2 padding)
    {
        ImVec2 textSize = ImGui::CalcTextSize(str);
        ImVec2 cursorStart = ImGui::GetCursorPos();
        ImGui::InvisibleButton("##TextPadded", textSize + ImVec2(padding.x * 2, padding.y * 2));
        ImVec2 cursorFinal = ImGui::GetCursorPos();
        ImGui::SetCursorPos(cursorStart + ImVec2(padding.x, padding.y));
        ImGui::Text(str);
        ImGui::SetCursorPos(cursorFinal);
    }

    static void TextUnderlined(const char* str)
    {
        ImGui::Text(str);
        AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_Text]);
    }

    static void TextOutlined(const char* fmt, ...)
    {
        ImVec2 pos = GetCursorPos();

        va_list args;
        va_start(args, fmt);
        pos.x += 1;
        pos.y += 1;
        SetCursorPos(pos);
        TextColoredV(ImVec4(0, 0, 0, 255), fmt, args);
        pos.x -= 1;
        pos.y -= 1;
        SetCursorPos(pos);
        TextV(fmt, args);
        va_end(args);
    }

    static void TextDisabledOutlined(const char* fmt, ...)
    {
        ImVec2 pos = GetCursorPos();

        va_list args;
        va_start(args, fmt);
        pos.x += 1;
        pos.y += 1;
        SetCursorPos(pos);
        TextColoredV(ImVec4(0, 0, 0, 255), fmt, args);
        pos.x -= 1;
        pos.y -= 1;
        SetCursorPos(pos);
        TextDisabledV(fmt, args);
        va_end(args);
    }

    static void TextColoredOutlined(const ImVec4& col, const char* fmt, ...)
    {
        ImVec2 pos = GetCursorPos();

        va_list args;
        va_start(args, fmt);
        pos.x += 1;
        pos.y += 1;
        SetCursorPos(pos);
        TextColoredV(ImVec4(0, 0, 0, 255), fmt, args);
        pos.x -= 1;
        pos.y -= 1;
        SetCursorPos(pos);
        TextColoredV(col, fmt, args);
        va_end(args);
    }

    static void TextWrappedOutlined(const char* fmt, ...)
    {
        ImVec2 pos = GetCursorPos();

        va_list args;
        va_start(args, fmt);
        pos.x += 1;
        pos.y += 1;
        SetCursorPos(pos);
        PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 255));
        TextWrappedV(fmt, args);
        ImGui::PopStyleColor();
        pos.x -= 1;
        pos.y -= 1;
        SetCursorPos(pos);
        TextWrappedV(fmt, args);
        va_end(args);
    }

    static bool TextLink(const char* name_, bool SameLineBefore_, bool SameLineAfter_)
    {
        bool clicked = false;

        if (true == SameLineBefore_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
        ImGui::Text(name_);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
            {
                clicked = true;
            }
            AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
        }
        else
        {
            AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
        }
        if (true == SameLineAfter_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }

        return clicked;
    }

    static ImVector<ImRect> s_GroupPanelLabelStack;

    static void BeginGroupPanel(const char* name, const ImVec2& size)
    {
        ImGui::BeginGroup();

        auto cursorPos = ImGui::GetCursorScreenPos();
        auto itemSpacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        auto frameHeight = ImGui::GetFrameHeight();
        ImGui::BeginGroup();

        ImVec2 effectiveSize = size;
        if (size.x < 0.0f)
            effectiveSize.x = ImGui::GetContentRegionAvailWidth();
        else
            effectiveSize.x = size.x;
        ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Text(name);
        auto labelMin = ImGui::GetItemRectMin();
        auto labelMax = ImGui::GetItemRectMax();
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
        ImGui::BeginGroup();

        //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

        ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
        ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
        ImGui::GetCurrentWindow()->Size.x -= frameHeight;

        auto itemWidth = ImGui::CalcItemWidth();
        ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

        s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
    }

    static void EndGroupPanel()
    {
        ImGui::PopItemWidth();

        auto itemSpacing = ImGui::GetStyle().ItemSpacing;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

        auto frameHeight = ImGui::GetFrameHeight();

        ImGui::EndGroup();

        //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

        ImGui::EndGroup();

        auto itemMin = ImGui::GetItemRectMin();
        auto itemMax = ImGui::GetItemRectMax();
        //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

        auto labelRect = s_GroupPanelLabelStack.back();
        s_GroupPanelLabelStack.pop_back();

        ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
        ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
        labelRect.Min.x -= itemSpacing.x;
        labelRect.Max.x += itemSpacing.x;
        for (int i = 0; i < 4; ++i)
        {
            switch (i)
            {
                // left half-plane
                case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
                    // right half-plane
                case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
                    // top
                case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
                    // bottom
                case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
            }

            ImGui::GetWindowDrawList()->AddRect(
                frameRect.Min, frameRect.Max,
                ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Separator)),
                halfFrame.x);

            ImGui::PopClipRect();
        }

        ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
        ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
        ImGui::GetCurrentWindow()->Size.x += frameHeight;

        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        ImGui::EndGroup();
    }
}

#endif