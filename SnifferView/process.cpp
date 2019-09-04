/*
 *	filename:  process.cpp
 *	author:	    lougd
 *	created:    2015-6-12 13:03
 *	version:    1.0.0.1
 *	desc:			查看进程信息的接口
 *  history:
*/
#include <WinSock2.h>
#include <Windows.h>
#include "../ComLib/common.h"
#include "view/netstat.h"
#include "resource.h"
#include "global.h"
#include "view/view.h"

static HWND s_process_dlg = NULL;
static HWND s_dec = NULL;
static HWND s_path = NULL;
static HWND s_ico = NULL;
static HICON s_ico_data = NULL;
static ProcessInfo s_process;

static VOID OnInitDialog(HWND hdlg, WPARAM wp, LPARAM lp)
{
	s_process_dlg = hdlg;
	s_dec = GetDlgItem(hdlg, IDC_ST_DEC);
	s_path = GetDlgItem(hdlg, IDC_EDT_PATH);
	s_ico = GetDlgItem(hdlg, IDC_PROCESS_ICO);
	if (lp == 0)
	{
		return;
	}
	ProcessInfo *info = (ProcessInfo *)lp;
	ProcessInfo msg = *info;
	s_process = msg;
	mstring ms;
	mstring show;
	show.format("%hs Pid:%d", msg.m_name.c_str(), msg.m_pid);
	SetWindowText(hdlg, show.c_str());

	show.clear();
	if(GetFileString(msg.m_path.c_str(), "FileDescription", ms))
	{
		show = ms;
	}
	show += "\r\n\r\n";

	if (GetFileString(msg.m_path.c_str(), "CompanyName", ms))
	{
		show += ms;
	}
	show += "\r\n\r\n";

	mstring uu;
	if (GetFileVersion(msg.m_path.c_str(), ms))
	{
		uu.format("版本:%hs", ms.c_str());
	}
	show += uu;
	SetWindowText(s_dec, show.c_str());

	show.format("%hs", msg.m_path.c_str());
	SetWindowTextA(s_path, show.c_str());

	SHFILEINFO mm = {0};
	if (0 != SHGetFileInfoA(msg.m_path.c_str(), 0, &mm, sizeof(mm), SHGFI_ICON))
	{
		s_ico_data = mm.hIcon;
	}
	SendMessage(s_ico, STM_SETICON, (WPARAM)s_ico_data, 0);
	CenterWindow(GetParent(hdlg), hdlg);
}

static VOID OnClose(HWND hdlg, WPARAM wp, LPARAM lp)
{
	EndDialog(hdlg, 0);
	if (s_ico_data)
	{
		DestroyIcon(s_ico_data);
		s_ico_data = NULL;
	}
	s_process_dlg = NULL;
}

static VOID OnCommand(HWND hdlg, WPARAM wp, LPARAM lp)
{
	DWORD id = LOWORD(wp);
	switch(id)
	{
	case IDC_BTN_PE:						//定位
		{
			mstring cmd;
			cmd.format("%hs,\"%hs\"", "/select", s_process.m_path.c_str());
			ShellExecuteA(NULL, "open", "explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
		}
		break;
	case  IDC_BTN_APPEND:				//附加
		break;
	case  IDC_BTN_NET:					//网络状态
		{
			HWND state = RunNetstatView(g_main_view, NULL);
			NetstatCmd cmd;
			cmd.m_cmd.format("%d", s_process.m_pid);
			SendMessageA(state, MSG_EXEC_CMD, 0, (LPARAM)&cmd);
			SendMessageA(hdlg, WM_CLOSE, 0, 0);
		}
		break;
	case  IDC_BTN_END:					//结束进程
		{
			mstring msg;
			msg.format("确定要结束 %hs 进程？", s_process.m_name.c_str());
			if (IDOK == MessageBoxA(hdlg, msg.c_str(), "确认", MB_OKCANCEL | MB_ICONWARNING))
			{
				HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, s_process.m_pid);
				if (process)
				{
					TerminateProcess(process, 0);
					CloseHandle(process);
					SendMessageA(hdlg, WM_CLOSE, 0, 0);
				}
				else
				{
					MessageBoxA(hdlg, "结束进程失败，可能是权限不够", "失败", MB_OK);
				}
			}
		}
		break;
	default:
		break;
	}
}

INT_PTR CALLBACK ProcessProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			OnInitDialog(hdlg, wp, lp);
		}
		break;
	case  WM_COMMAND:
		{
			OnCommand(hdlg, wp, lp);
		}
		break;
	case  WM_CLOSE:
		{
			OnClose(hdlg, wp, lp);
		}
		break;
	}
	return 0;
}

VOID ShowProcessStat(HWND parent, ProcessInfo info)
{
	if (s_process_dlg)
	{
		SendMessageA(s_process_dlg, WM_CLOSE, 0, 0);
	}
	s_process_dlg = NULL;
	DialogBoxParamA(g_m, MAKEINTRESOURCEA(IDD_PROCESS), parent, ProcessProc, (LPARAM)&info);
}