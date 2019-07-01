#include <WinSock2.h>
#include <Shlwapi.h>
#include <mstring.h>
#include <common.h>
#include "help.h"
#include "resource.h"

static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	char name[MAX_PATH] = {0x00};
	GetClassNameA(hwnd, name, MAX_PATH);
	if (0 == lstrcmpiA(name, "edit"))
	{
		SendMessage(hwnd, EM_SETREADONLY, 1, 0);
		return FALSE;
	}
	return TRUE;
}

VOID WINAPI ShowHelpView(HWND parent)
{
	char path[MAX_PATH] = {0x00};
	GetTempPathA(MAX_PATH, path);
	PathAppendA(path, HELP_FILE_NAME);
	ReleaseRes(path, IDR_HTML_HELP, RT_HTML);
	ShellExecuteA(NULL, "open", path,"","", SW_MAXIMIZE); 
}

//VOID WINAPI ShowAboutView(HWND parent)
//{
//	srand(GetTickCount());
//	mstring wnd;
//	char path[MAX_PATH] = {0x00};
//	GetTempPathA(MAX_PATH, path);
//	PathAppendA(path, ABOUT_FILE_NAME);
//	wnd.format("%hs - 记事本", ABOUT_FILE_NAME);
//	ReleaseRes(path, IDI_ABOUT_FILE, "txt");
//	mstring cmd;
//	cmd.format("notepad.exe \"%hs\"", path);
//	PROCESS_INFORMATION pi;  
//	STARTUPINFOA si = {sizeof(si)};  
//	BOOL ret = CreateProcessA(
//		NULL,
//		(LPSTR)cmd.c_str(),
//		NULL,
//		NULL,
//		FALSE,
//		0,
//		NULL,
//		NULL,
//		&si,
//		&pi
//		);  
//	if (!ret)
//	{
//		return;
//	}
//
//	if (pi.hProcess)
//	{
//		CloseHandle(pi.hProcess);
//	}
//
//	if (pi.hThread)
//	{
//		CloseHandle(pi.hThread);
//	}
//
//	HWND hwnd = NULL;
//	int count = 0;
//	while(hwnd == NULL && count < 200)
//	{
//		hwnd = FindWindowA(NULL, wnd.c_str());
//		Sleep(10);
//		count++;
//	}
//
//	if (!hwnd)
//	{
//		return;
//	}
//	SetWindowTextA(hwnd, "关于 SnifferView");
//	EnumChildWindows(hwnd, EnumChildProc, NULL);
//	SetForegroundWindow(hwnd);
//	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 900, 600, SWP_NOMOVE);
//	CenterWindow(parent, hwnd);
//}