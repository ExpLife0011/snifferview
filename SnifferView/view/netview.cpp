#include <WinSock2.h>
#include <list>
#include "../../ComLib/common.h"
#include "netview.h"
#include "view.h"
#include "../global.h"
#include "../resource.h"
#include "../servers.h"
#include "../config.h"

using namespace std;

static HWND s_net_list = NULL;
static HWND s_group = NULL;
static HWND s_mac = NULL;
static HWND s_ip = NULL;
static HWND s_mask = NULL;
static HWND s_gatway = NULL;
static HWND s_dhcp = NULL;
static HWND s_check = NULL;
static int s_current_sel = 0;

static VOID OnInitDialog(HWND hdlg, WPARAM wp, LPARAM lp)
{
	RefushSnifferServers();
	CenterWindow(g_main_view, hdlg);
	ShowWindow(hdlg, SW_SHOW);
	s_net_list = GetDlgItem(hdlg, IDC_COM_NETS);
	s_group = GetDlgItem(hdlg, IDC_ST_NETCARD_MSG);
	s_mac = GetDlgItem(hdlg, IDC_ST_MAC);
	s_ip = GetDlgItem(hdlg, IDC_ST_IP);
	s_mask = GetDlgItem(hdlg, IDC_ST_MASK);
	s_dhcp = GetDlgItem(hdlg, IDC_ST_DHCP);
	s_gatway = GetDlgItem(hdlg, IDC_ST_GATWAY);
	s_check = GetDlgItem(hdlg, IDC_CH_SELECT);

	LOCK_NET;
	vector<AdapterMsg>::iterator itm;
	int its = 0;
	for (itm = g_adapters.begin() ; itm != g_adapters.end() ; itm++, its++)
	{
		SendMessageA(s_net_list, CB_INSERTSTRING, its, (LPARAM)(itm->m_desc.c_str()));
	}
	UNLOCK_NET;
	SendMessageA(s_net_list, CB_SETCURSEL, 0, 0);
	WPARAM mv = 0;
	mv |= (CBN_SELCHANGE << 16);
	mv |= (IDC_COM_NETS);
	SendMessageA(hdlg, WM_COMMAND, mv, 0);
}

static VOID OnCommand(HWND hdlg, WPARAM wp, LPARAM lp)
{
	DWORD id = LOWORD(wp);
	DWORD ms = HIWORD(wp);
	if (IDC_COM_NETS == id)
	{
		if (CBN_SELCHANGE == ms)
		{
			int sel = SendMessageA(s_net_list, CB_GETCURSEL, 0, 0);
			if (-1 != sel)
			{
				s_current_sel = sel;
				LOCK_NET;
				if (sel > (int)g_adapters.size())
				{
					UNLOCK_NET;
					return;
				}
				mstring vv;
				vv.format("%hs-%hs", g_adapters[sel].m_desc.c_str(), g_adapters[sel].m_type.c_str());
				SetWindowTextA(s_group, vv.c_str());
				SetWindowTextA(s_mac, g_adapters[sel].m_mac.c_str());
				SetWindowTextA(s_ip, g_adapters[sel].m_ip.c_str());
				SetWindowTextA(s_mask, g_adapters[sel].m_mask.c_str());
				SetWindowTextA(s_gatway, g_adapters[sel].m_gateway.c_str());
				if (g_adapters[sel].m_dhcp_enable)
				{
					SetWindowTextA(s_dhcp, "ÊÇ");
				}
				else
				{
					SetWindowTextA(s_dhcp, "·ñ");
				}

				mstring name = g_adapters[s_current_sel].m_name;
				if (g_sniffer_servers.end() != g_sniffer_servers.find(name))
				{
					SendMessageA(s_check, BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendMessageA(s_check, BM_SETCHECK, BST_UNCHECKED, 0);
				}
				UNLOCK_NET;
				InvalidateRect(hdlg, NULL, TRUE);
			}
		}
	}

	if (IDC_CH_SELECT == id)
	{
		LOCK_NET;
		if(s_current_sel > (int)(g_adapters.size() - 1))
		{
			UNLOCK_NET;
			return ;
		}

		bool check = (BST_CHECKED == SENDMSG(s_check, BM_GETCHECK));
		mstring name = g_adapters[s_current_sel].m_name;
		if (check)
		{
			if (!IsNetcardSniffer(name.c_str()))
			{
				StartSnifferServer(name.c_str());
			}
			InsertNetcard(name.c_str());
		}

		if (!check)
		{
			if (IsNetcardSniffer(name.c_str()))
			{
				SuspendSnifferServer(name.c_str());
			}
			DeleteNetcard(name.c_str());
		}
		SaveFilterConfig();
		UNLOCK_NET;
	}

	if (IDC_BTN_ALL == id)
	{
		AddAllNetcard();
		StartAllSnifferServers();
		LOCK_NET;
		if(s_current_sel > (int)(g_adapters.size() - 1))
		{
			UNLOCK_NET;
			return;
		}
		bool check = (BST_CHECKED == SENDMSG(s_check, BM_GETCHECK));
		mstring name = g_adapters[s_current_sel].m_name;
		if(g_sniffer_servers.end() != g_sniffer_servers.find(name))
		{
			SendMessageA(s_check, BM_SETCHECK, BST_CHECKED, 0);
		}
		SaveFilterConfig();
		UNLOCK_NET;
		EndDialog(hdlg, 0);
	}
}

INT_PTR CALLBACK NetCardConfigProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case  WM_INITDIALOG:
		{
			OnInitDialog(hdlg, wp, lp);
		}
		break;
	case WM_COMMAND:
		{
			OnCommand(hdlg, wp, lp);
		}
		break;
	case  WM_CLOSE:
		EndDialog(hdlg, 0);
		break;
	default:
		break;
	}
	return 0;
}

VOID ShowNetConfigView()
{
	DialogBoxA(g_m, MAKEINTRESOURCEA(IDD_NETCARD_CONFIG), g_main_view, NetCardConfigProc);
}