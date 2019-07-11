#include <Windows.h>
#include <string>
#include <Shlwapi.h>
#include "StrUtil.h"

using namespace std;

typedef string strutf8;
strutf8 ToUtf8W(const wstring &str);
wstring ToWideChar(const string &str);
string ToMultiByte(const wstring &str);
wstring ToCommonW(const strutf8 &str);

strutf8 ToUtf8A(const string &str)
{
    return ToUtf8W(ToWideChar(str));
}

strutf8 ToUtf8W(const wstring &str)
{
    strutf8 ret;

    int count = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size() + 1, NULL, 0, NULL, NULL);

    if (count > 0)
    {
        char *buffer = new char[count];

        if (buffer != 0)
        {
            WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size() + 1, buffer, count, NULL, NULL);
            ret.append(buffer, count - 1);

            delete []buffer;
        }
    }

    return ret;
}

string ToCommonA(const strutf8 &str)
{
    return ToMultiByte(ToCommonW(str));
}

wstring ToCommonW(const strutf8 &str)
{
    wstring ret;

    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1, NULL, 0);

    if (count > 0)
    {
        wchar_t *buffer = new wchar_t[count];

        if (buffer != 0)
        {
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1, buffer, count);
            ret.append(buffer, count - 1);

            delete []buffer;
        }
    }

    return ret;
}

string ToMultiByte(const wstring &str)
{
    string ret;

    int count = WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size() + 1, NULL, 0, NULL, NULL);

    if (count > 0)
    {
        char *buffer = new char[count];

        if (buffer != 0)
        {
            WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size() + 1, buffer, count, NULL, NULL);
            ret.append(buffer, count - 1);

            delete []buffer;
        }
    }

    return ret;
}

wstring ToWideChar(const string &str)
{
    wstring ret;

    int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size() + 1, NULL, 0);

    if (count > 0)
    {
        wchar_t *buffer = new wchar_t[count];

        if (buffer != 0)
        {
            MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size() + 1, buffer, count);
            ret.append(buffer, count - 1);

            delete []buffer;
        }
    }

    return ret;
}

string __stdcall AtoU(const string &str) {
    return ToUtf8A(str);
}

string __stdcall UtoA(const string &str) {
    return ToCommonA(str);
}

wstring __stdcall AtoW(const string &str) {
    return ToWideChar(str);
}

string __stdcall WtoA(const wstring &wstr) {
    return ToMultiByte(wstr);
}

wstring __stdcall UtoW(const string &str) {
    return ToCommonW(str);
}

string __stdcall WtoU(const wstring &wstr) {
    return ToUtf8W(wstr);
}

string __stdcall FormatA(const char *format, ...) {
    char szText[2048] = {0};
    va_list val;

    va_start(val, format);
    wvnsprintfA(szText, RTL_NUMBER_OF(szText), format, val);
    va_end(val);
    return szText;
}

wstring __stdcall FormatW(const wchar_t *format, ...)
{
    wchar_t szText[2048] = {0};
    va_list val;

    va_start(val, format);
    wvnsprintfW(szText, RTL_NUMBER_OF(szText), format, val);
    va_end(val);

    return szText;
}

list<mstring> SplitStrA(const mstring &str, const mstring &split) {
    list<mstring> result;
    if (split.empty())
    {
        return result;
    }

    size_t last = 0;
    size_t pos = 0;
    while (true) {
        pos = str.find(split, last);

        if (pos == mstring::npos)
        {
            if (str.size() > last)
            {
                result.push_back(str.substr(last));
            }
            return result;
        }

        if (pos > last)
        {
            result.push_back(str.substr(last, pos - last));
        }
        last = pos + split.size();
    }
}

list<ustring> SplitStrW(const ustring &str, const ustring &split) {
    list<ustring> result;
    if (split.empty())
    {
        return result;
    }

    size_t last = 0;
    size_t pos = 0;
    while (true) {
        pos = str.find(split, last);

        if (pos == ustring::npos)
        {
            if (str.size() > last)
            {
                result.push_back(str.substr(last));
            }
            return result;
        }

        if (pos > last)
        {
            result.push_back(str.substr(last, pos - last));
        }
        last = pos + split.size();
    }
}