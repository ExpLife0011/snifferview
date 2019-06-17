#pragma once
#include <Windows.h>

//打印调试信息
VOID PrintDbgInternal(LPCWSTR wszTarget, LPCSTR szFile, DWORD dwLine, LPCWSTR wszFormat, ...);
#define dp(f, ...) PrintDbgInternal(L"SyntaxView", __FILE__, __LINE__, f, ##__VA_ARGS__)