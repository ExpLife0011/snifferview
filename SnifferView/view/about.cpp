#include <WinSock2.h>
#include <Windows.h>
#include <common.h>
#include "../global.h"
#include "../resource.h"
#include "view.h"

HWND s_about_ico = NULL;
HWND s_about_ver = NULL;
HWND s_about_msg =  NULL;

const char *s_about =\
"  SnifferView是一个网络分析工具，体积小巧，速度快，支持类似抓包工具\
Wireshark的过滤语句，支持Tcp，Udp，Http等协议的分析\
，支持网络数据的导入和导出，\
支持应用程序网络状态的查看分析，支持根据窗体获取应用程序的网络状态等功能。";

static VOID OnInitDialog(HWND hdlg, WPARAM wp, LPARAM lp)
{
	CenterWindow(GetParent(hdlg), hdlg);
	s_about_ico = GetDlgItem(hdlg, IDC_ABOUT_ICO);
	s_about_ver = GetDlgItem(hdlg, IDC_ABOUT_VER);
	s_about_msg = GetDlgItem(hdlg, IDC_EDT_ABOUT);
	HICON ico = LoadIconA(g_m, MAKEINTRESOURCEA(IDI_MAIN));
	SendMessage(s_about_ico, STM_SETICON, (WPARAM)ico, 0);
	mstring msg;
	mstring ver;
	char path[MAX_PATH] = {0x00};
	GetModuleFileNameA(NULL, path, MAX_PATH);
	GetFileVersion(path, ver);
	msg.format("SnifferView 版本:%hs\r\n\r\n联系我：lougdhr@126.com", ver.c_str());
	SetWindowTextA(s_about_ver, msg.c_str());
	SetWindowTextA(s_about_msg, s_about);
	DestroyIcon(ico);
}

INT_PTR CALLBACK DlgAbout(HWND hdlg,UINT msg, WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case  WM_INITDIALOG:
		{
			OnInitDialog(hdlg, wp, lp);
		}
		break;
	case  WM_CLOSE:
		{
			EndDialog(hdlg, 0);
		}
		break;
	default:
		break;
	}
	return 0;
}


VOID ShowAbout()
{
	DialogBoxA(g_m, MAKEINTRESOURCEA(IDD_ABOUT), g_main_view, DlgAbout);
}