#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

VOID PrintDbgInternal(LPCWSTR wszTarget, LPCSTR szFile, DWORD dwLine, LPCWSTR wszFormat, ...)
{
    WCHAR wszFormat1[1024] = {0};
    WCHAR wszFormat2[1024] = {0};
    lstrcpyW(wszFormat1, L"[%ls][%hs.%d]%ls");
    StrCatW(wszFormat1, L"\n");
    wnsprintfW(wszFormat2, RTL_NUMBER_OF(wszFormat2), wszFormat1, wszTarget, szFile, dwLine, wszFormat);

    WCHAR wszLogInfo[1024];
    va_list vList;
    va_start(vList, wszFormat);
    wvnsprintfW(wszLogInfo, sizeof(wszLogInfo), wszFormat2, vList);
    va_end(vList);
    OutputDebugStringW(wszLogInfo);
}