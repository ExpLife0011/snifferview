#pragma once
#include <Windows.h>

//��ӡ������Ϣ
VOID PrintDbgInternal(LPCWSTR wszTarget, LPCSTR szFile, DWORD dwLine, LPCWSTR wszFormat, ...);
#define dp(f, ...) PrintDbgInternal(L"SnifferView", __FILE__, __LINE__, f, ##__VA_ARGS__)