#include "gdcharconv.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <algorithm>

using namespace std;

// 使用 new (std::nothrow) 后，DLL多了一个导出函数，所以不要这个了
/*
#if _MSC_VER > 1200
#  define NEW new (std::nothrow)
#else
#  define NEW new
#endif
*/
#define NEW new

strutf8 ToUtf8A(const string &str)
{
	return ToUtf8W(ToWideChar(str));
}

strutf8 ToUtf8W(const wstring &str)
{
	strutf8 ret;

	int count = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);

	if (count > 0)
	{
		char *buffer = NEW char[count];

		if (buffer != 0)
		{
			WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer, count, NULL, NULL);
			ret = buffer;

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

	int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

	if (count > 0)
	{
		wchar_t *buffer = NEW wchar_t[count];

		if (buffer != 0)
		{
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, count);
			ret = buffer;

			delete []buffer;
		}
	}

	return ret;
}

string ToMultiByte(const wstring &str)
{
	string ret;

	int count = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, NULL, 0, NULL, NULL);

	if (count > 0)
	{
		char *buffer = NEW char[count];

		if (buffer != 0)
		{
			WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, buffer, count, NULL, NULL);
			ret = buffer;

			delete []buffer;
		}
	}

	return ret;
}

wstring ToWideChar(const string &str)
{
	wstring ret;

	int count = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	if (count > 0)
	{
		wchar_t *buffer = NEW wchar_t[count];

		if (buffer != 0)
		{
			MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, count);
			ret = buffer;

			delete []buffer;
		}
	}

	return ret;
}

std::string fmt(const char *format, ...)
{
    char szText[2048] = {0};
    va_list val;

    va_start(val, format);
    wvnsprintfA(szText, RTL_NUMBER_OF(szText), format, val);
    va_end(val);

    return szText;
}

std::wstring fmt(const wchar_t *format, ...)
{
    wchar_t szText[2048] = {0};
    va_list val;

    va_start(val, format);
    wvnsprintfW(szText, RTL_NUMBER_OF(szText), format, val);
    va_end(val);

    return szText;
}

template<typename Ch, class Conv>
basic_string<Ch> _convCase(const basic_string<Ch> &str, Conv conv)
{
    basic_string<Ch> ret = str;
    transform(str.begin(), str.end(), ret.begin(), conv);
    return ret;
}

string toLower(const string &str)
{
    return _convCase<char>(str, tolower);
}

wstring toLower(const wstring &str)
{
    return _convCase<wchar_t>(str, towlower);
}

string toUpper(const string &str)
{
    return _convCase<char>(str, toupper);
}

wstring toUpper(const wstring &str)
{
    return _convCase<wchar_t>(str, towupper);
}
