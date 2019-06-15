#include <WinSock2.h>
#include <Shlwapi.h>
#include <common.h>
#include "cfgview.h"
#include "view.h"
#include "../servers.h"
#include "../global.h"
#include "../resource.h"
#include "../filter.h"
#include "../config.h"
#include "../help.h"
#include "../analysis.h"

#define     MSG_CONFIG_SHOW_EDIT_RETURN                 (WM_USER + 1001)
#define     MSG_CONFIG_FILTER_EDIT_RETURN               (WM_USER + 1002)
#define     MSG_SHOW_SYNTAX_ERROR                       (WM_USER + 2001)
#define     MSG_FILTER_SYNTAX_ERROR                     (WM_USER + 2002)
#define     MSG_RULES_SUCESS                            (WM_USER + 3001)

static COLORREF s_error_bk_colour = RGB(255, 0, 0);
static COLORREF s_right_bk_colour = RGB(255, 255, 255);
static BOOL s_error_mark = FALSE;

#define  SetError(msg)	SetWindowTextA(s_edit_msg, msg);\
									s_error_mark = TRUE;\
									InvalidateRect(s_edit_msg, NULL, TRUE)
#define  SetMsg(msg)	SetWindowTextA(s_edit_msg, msg);\
									s_error_mark = FALSE;\
									InvalidateRect(s_edit_msg, NULL, TRUE)


static PWIN_PROC g_show_proc = NULL;
static PWIN_PROC g_filter_proc = NULL;
static HWND s_config_view = NULL;

static HBRUSH s_brush;
static HWND s_ck_net;
static HWND s_ck_host;
static HWND s_ck_m;
static HWND s_edit_show;
static HWND s_edit_filter;
static HWND s_edit_msg;
static HWND s_rd_ip;
static HWND s_rd_tcp;
static HWND s_rd_user;
static HWND s_ck_file;
static HWND s_hMaxCount = NULL;

static ULONGLONG _GetCfgMaxCount()
{
    ustring wstrText;

    GetWindowTextW(s_hMaxCount, wstrText.alloc(128), 128);

    wstrText.setbuffer();

    return _wtoi64(wstrText.c_str());
}

static VOID _SetCfgMaxCount(ULONGLONG llCount)
{
    ustring wstrText;

    wstrText.format(L"%I64u", llCount);

    SetWindowTextW(s_hMaxCount, wstrText.c_str());
}

LRESULT CALLBACK ShowEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (WM_KEYDOWN == msg)
	{
		if (0x0d == wp)
		{
			SENDMSG(s_config_view, MSG_CONFIG_SHOW_EDIT_RETURN);
			return 0;
		}
	}
	return CallWindowProc(g_show_proc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK FilterEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (WM_KEYDOWN == msg)
	{
		if (0x0d == wp)
		{
			SENDMSG(s_config_view, MSG_CONFIG_FILTER_EDIT_RETURN);
			return 0;
		}
	}
	return CallWindowProc(g_filter_proc, hwnd, msg, wp, lp);
}

VOID OnCfgviewReturn()
{
	BOOL filter_refush = FALSE;
	BOOL show_refush = FALSE;
	BOOL net = (BST_CHECKED== SENDMSG(s_ck_net, BM_GETCHECK));
	BOOL mark = (BST_CHECKED != SENDMSG(s_ck_m, BM_GETCHECK));
	if (net != g_net_mark || mark != g_str_mark)
	{
		filter_refush = TRUE;
	}

	list<FilterRules> filter_rules;
	list<FilterRules> show_rules;
	int length = GetWindowTextLengthA(s_edit_filter);
	mstring filter;
	GetWindowTextA(s_edit_filter, filter.alloc(length + 1), length + 1);
	filter.setbuffer();
	filter.trimleft();
	filter.trimright();
	if (filter != g_filter_string)
	{
		//检查过滤表达式的语法错误
		if (filter.size() > 0 && !RulesCompile(filter.c_str(), filter_rules))
		{
			//filter error
			SENDMSG(s_config_view, MSG_FILTER_SYNTAX_ERROR);
			return;
		}
	}

	mstring show;
	length = GetWindowTextLengthA(s_edit_show);
	GetWindowTextA(s_edit_show, show.alloc(length + 1), length + 1);
	show.setbuffer();
	show.trimleft();
	show.trimright();
	if (show != g_show_string)
	{
		//检查显示表达式的语法错误
		if (show.size() > 0 && !RulesCompile(show.c_str(), show_rules))
		{
			//show error
			SENDMSG(s_config_view, MSG_SHOW_SYNTAX_ERROR);
			return;
		}
	}
	
	g_net_mark = net;
	g_str_mark = mark;
	g_show_string = show;
	g_filter_string = filter;
    SetMaxPacketCount(_GetCfgMaxCount());

	if (filter.size() > 0)
	{
		RulesCompile(filter.c_str(), filter_rules);
		if (0 == filter_rules.size())
		{
			SetError("过滤规则有冲突");
			return;
		}
	}
	
	if (show.size() > 0)
	{
		RulesCompile(show.c_str(), show_rules);
		if (0 == show_rules.size())
		{
			SetError("显示规则有冲突");
			return;
		}
	}

	SaveFilterConfig();
	if (IsFilterRulesDif(filter_rules))
	{
		filter_refush = TRUE;
	}

	if (IsShowRulesDif(show_rules))
	{
		show_refush = TRUE;
	}

	if (show_refush)
	{
		SetMsg("规则生效");
		ChangeShowRules(show_rules);
		RecheckShowPacket();
	}
	NoticefSnifferViewRefush();
	SENDMSG(s_config_view, MSG_RULES_SUCESS);

    SetMaxPacketCount(_GetCfgMaxCount());
}

VOID OnEditShowReturn()
{
	OnCfgviewReturn();
}

VOID OnEditFilterReturn()
{
	OnCfgviewReturn();
}

VOID InitCfgParams()
{
	if (g_net_mark)
	{
		SendMessageA(s_ck_net, BM_SETCHECK, 1, 0);
		SendMessageA(s_ck_host, BM_SETCHECK, 0, 0);
	}
	else
	{
		SendMessageA(s_ck_net, BM_SETCHECK, 0, 0);
		SendMessageA(s_ck_host, BM_SETCHECK, 1, 0);
	}

	if (!g_str_mark)
	{
		SendMessageA(s_ck_m, BM_SETCHECK, 1, 0);
	}
	else
	{
		SendMessageA(s_ck_m, BM_SETCHECK, 0, 0);
	}

	SetWindowTextA(s_edit_show, g_show_string.c_str());
	SetWindowTextA(s_edit_filter, g_filter_string.c_str());
	switch(g_hex_hight)
	{
	case  em_ip_header:
		SendMessageA(s_rd_ip, BM_SETCHECK, 1, 0);
		SendMessageA(s_rd_tcp, BM_SETCHECK, 0, 0);
		SendMessageA(s_rd_user, BM_SETCHECK, 0, 0);
		break;
	case  em_tcp_udp_icmp_header:
		SendMessageA(s_rd_ip, BM_SETCHECK, 0, 0);
		SendMessageA(s_rd_tcp, BM_SETCHECK, 1, 0);
		SendMessageA(s_rd_user, BM_SETCHECK, 0, 0);
		break;
	case  em_user_data:
		SendMessageA(s_rd_ip, BM_SETCHECK, 0, 0);
		SendMessageA(s_rd_tcp, BM_SETCHECK, 0, 0);
		SendMessageA(s_rd_user, BM_SETCHECK, 1, 0);
		break;
	}

	SendMessageA(s_edit_show, EM_SETSEL, g_show_string.size(), g_show_string.size());
	SendMessageA(s_edit_filter, EM_SETSEL, g_filter_string.size(), g_filter_string.size());

	if (CheckFileRelation(SNIFFER_DATA_EXT, SNIFFER_DATA_KEY))
	{
		SendMessageA(s_ck_file, BM_SETCHECK, 1, 0);
		EnableWindow(s_ck_file, FALSE);
	}
}

INT_PTR CALLBACK ConfigProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	static HBRUSH s_error_brush;
	static HBRUSH s_right_brush;
	switch(msg)
	{
	case  WM_INITDIALOG:
		{
			CentreWindow(g_main_view, hdlg);
			ShowWindow(hdlg, SW_SHOW);
			s_error_mark = FALSE;
			s_config_view = hdlg;
			s_ck_net = GetDlgItem(hdlg, IDC_RDO_N);
			s_ck_host = GetDlgItem(hdlg, IDC_RDO_H);
			s_ck_m = GetDlgItem(hdlg, IDC_CK_SREM);
			s_edit_show = GetDlgItem(hdlg, IDC_EDT_SHOW);
			s_edit_filter = GetDlgItem(hdlg, IDC_EDT_FILTER);
			s_edit_msg = GetDlgItem(hdlg, IDC_EDT_MSG);
			s_rd_ip = GetDlgItem(hdlg, IDC_RD_IP);
			s_rd_tcp = GetDlgItem(hdlg, IDC_RD_TCP);
			s_rd_user = GetDlgItem(hdlg, IDC_RD_USER);
			s_ck_file = GetDlgItem(hdlg, IDC_CK_FILE);

			InitCfgParams();
			g_show_proc = (PWIN_PROC)SetWindowLong(s_edit_show, GWL_WNDPROC, (long)ShowEditProc);
			g_filter_proc = (PWIN_PROC)SetWindowLong(s_edit_filter, GWL_WNDPROC, (long)FilterEditProc);
			s_error_brush = CreateSolidBrush(s_error_bk_colour);
			s_right_brush = CreateSolidBrush(s_right_bk_colour);
            s_hMaxCount = GetDlgItem(s_config_view, IDC_EDT_MAX);

            _SetCfgMaxCount(GetMaxPacketCount());

			if (em_show == lp)
			{
				SetFocus(s_edit_show);
			}

			if (em_filter == lp)
			{
				SetFocus(s_edit_filter);
			}

			if (g_work_state == em_analysis)
			{
				EnableWindow(s_edit_filter, FALSE);
			}
		}
		break;
	case  WM_COMMAND:
		{
			DWORD id = LOWORD(wp);
			if (id == IDC_BTN_OK)
			{
				OnCfgviewReturn();
			}

			if (id == IDC_RD_IP)
			{
				if (IsChecked(s_rd_ip))
				{
					g_hex_hight = em_ip_header;
					SaveFilterConfig();
				}
			}
			
			if (id == IDC_RD_TCP)
			{
				if (IsChecked(s_rd_tcp))
				{
					g_hex_hight = em_tcp_udp_icmp_header;
					SaveFilterConfig();
				}
			}

			if (id == IDC_RD_USER)
			{
				if (IsChecked(s_rd_user))
				{
					g_hex_hight = em_user_data;
					SaveFilterConfig();
				}
			}

			if (id == IDC_CK_FILE)
			{
				if (IsChecked(s_ck_file))
				{
					if (!CheckFileRelation(SNIFFER_DATA_EXT, SNIFFER_DATA_KEY))
					{
						mstring cmd;
						GetModuleFileNameA(NULL, cmd.alloc(MAX_PATH), MAX_PATH);
						cmd.setbuffer();
						mstring ico(cmd.c_str());
						cmd += " /f";
						ico += ",1";
						RegisterFileRelation(SNIFFER_DATA_EXT, cmd.c_str(), SNIFFER_DATA_KEY, ico.c_str(), SNIFFER_DATA_DEC);
					}

					if (!CheckFileRelation(SNIFFER_DATA_EXT, SNIFFER_DATA_KEY))
					{
						mstring error;
						error.format("关联数据文件失败，lasterror=%d", GetLastError());
						SetError(error.c_str());
						SendMessageA(s_ck_file, BM_SETCHECK, 0, 0);
						EnableWindow(s_ck_file, TRUE);
					}
					else
					{
						SetMsg("关联数据文件成功");
						EnableWindow(s_ck_file, FALSE);
					}
				}
			}

			char path[MAX_PATH];
			GetTempPathA(MAX_PATH, path);
			PathAppendA(path, HELP_FILE_NAME);
			if (id == IDC_BTN_HELP)
			{
				ShowHelpView(hdlg);
			}

			if (id == IDC_BTN_V)
			{
				ShowHelpView(hdlg);
			}
		}
		break;
	case  MSG_CONFIG_SHOW_EDIT_RETURN:
		{
			OnEditShowReturn();
		}
		break;
	case  MSG_CONFIG_FILTER_EDIT_RETURN:
		{
			OnEditFilterReturn();
		}
		break;
	case  MSG_FILTER_SYNTAX_ERROR:
		{
			SetError("过滤规则语法错误");
		}
		break;
	case  MSG_SHOW_SYNTAX_ERROR:
		{
			SetError("显示规则语法错误");
		}
		break;
	case  MSG_RULES_SUCESS:
		{
			SENDMSG(hdlg, WM_CLOSE);
		}
		break;
	case  WM_CTLCOLORSTATIC:
		{
			if (s_edit_msg == (HWND)lp)
			{
				HDC dc = (HDC)wp;
				if (s_error_mark)
				{
					SetBkColor(dc, s_error_bk_colour);
					return (INT_PTR)s_error_brush;
				}
			}
		}
		break;
	case  WM_CLOSE:
		{
			if (s_error_brush)
			{
				DeleteObject(s_error_brush);
			}

			if (s_right_brush)
			{
				DeleteObject(s_right_brush);
			
			}
			EndDialog(hdlg, 0);
		}		
		break;
	default:
		break;
	}
	return 0;
}

VOID ShowConfigView(HWND parent, LPARAM lp)
{
	DialogBoxParamA(g_m, MAKEINTRESOURCEA(IDD_CONFIG), parent, ConfigProc, lp);
}