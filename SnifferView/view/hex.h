#pragma  once
#include <Windows.h>
#include "../common/mstring.h"

VOID WINAPI SetData(const BYTE *data, size_t length);

VOID WINAPI SetHighlighted(UINT begin, UINT end);

HWND WINAPI CreateHexView(HWND parent, int width, int hight);

VOID WINAPI DestroyHexView();

VOID WINAPI GetHexSelectData(std::mstring &strData);