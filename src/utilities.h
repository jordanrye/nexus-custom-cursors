#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

#include "imgui/imgui.h"
#include "imgui_extensions.h"

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

#endif /* UTILITIES_H */
