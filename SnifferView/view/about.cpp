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
"  SnifferView��һ������������ߣ����С�ɣ��ٶȿ죬֧������ץ������\
Wireshark�Ĺ�����䣬֧��Tcp��Udp��Http��Э��ķ���\
��֧���������ݵĵ���͵�����\
֧��Ӧ�ó�������״̬�Ĳ鿴������֧�ָ��ݴ����ȡӦ�ó��������״̬�ȹ��ܡ�";

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
	msg.format("SnifferView �汾:%hs\r\n\r\n��ϵ�ң�lougdhr@126.com", ver.c_str());
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