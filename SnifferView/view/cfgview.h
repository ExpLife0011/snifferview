#pragma  once 
#include <Windows.h>

enum ConfingViewState
{
	em_show,
	em_filter
};

VOID ShowConfigView(HWND parent, LPARAM lp = em_show);