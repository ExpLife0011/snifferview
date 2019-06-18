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

static HWND s_config_view = NULL;
static HWND s_ck_net;
static HWND s_ck_host;
static HWND s_ck_m;
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
    
    g_net_mark = net;
    g_str_mark = mark;
    SetMaxPacketCount(_GetCfgMaxCount());

    SaveFilterConfig();
    EndDialog(s_config_view, 0);
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

    if (CheckFileRelation(SNIFFER_DATA_EXT, SNIFFER_DATA_KEY))
    {
        SendMessageA(s_ck_file, BM_SETCHECK, 1, 0);
        EnableWindow(s_ck_file, FALSE);
    }
    _SetCfgMaxCount(GetMaxPacketCount());
}

INT_PTR CALLBACK ConfigProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
    case  WM_INITDIALOG:
        {
            CentreWindow(g_main_view, hdlg);
            ShowWindow(hdlg, SW_SHOW);
            s_config_view = hdlg;
            s_ck_net = GetDlgItem(hdlg, IDC_RDO_N);
            s_ck_host = GetDlgItem(hdlg, IDC_RDO_H);
            s_ck_m = GetDlgItem(hdlg, IDC_CK_SREM);
            s_rd_ip = GetDlgItem(hdlg, IDC_RD_IP);
            s_rd_tcp = GetDlgItem(hdlg, IDC_RD_TCP);
            s_rd_user = GetDlgItem(hdlg, IDC_RD_USER);
            s_ck_file = GetDlgItem(hdlg, IDC_CK_FILE);
            s_hMaxCount = GetDlgItem(s_config_view, IDC_EDT_MAX);

            InitCfgParams();
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
                        SendMessageA(s_ck_file, BM_SETCHECK, 0, 0);
                        EnableWindow(s_ck_file, TRUE);
                    }
                    else
                    {
                        EnableWindow(s_ck_file, FALSE);
                    }
                }
            }

            if (id == IDC_BTN_RESET)
            {
                ResetFilterConfig();
                InitCfgParams();
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

VOID ShowConfigView(HWND parent, LPARAM lp)
{
    DialogBoxParamA(g_m, MAKEINTRESOURCEA(IDD_CONFIG), parent, ConfigProc, lp);
}