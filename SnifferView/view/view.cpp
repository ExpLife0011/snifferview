#include <WinSock2.h>
#include <Windows.h>
#include <shlobj.h>
#include <WtsApi32.h>
#include <Commctrl.h>
#include <Shlwapi.h>
#include <common.h>
#include <mstring.h>
#include <winsize.h>
#include "hex.h"
#include "view.h"
#include "netview.h"
#include "cfgview.h"
#include "StreamDlg.h"
#include "about.h"
#include "netstat.h"
#include "../filter.h"
#include "../resource.h"
#include "../analysis.h"
#include "../global.h"
#include "../dump.h"
#include "../help.h"
#include "../servers.h"
#include "../PacketUnique.h"
#include "../process.h"
#include "../filter.h"
#include "../FileCache.h"
#include "../PacketCache.h"
#include "ProgressDlg.h"
#include "../common/StrUtil.h"

enum MainViewMode {
    em_mode_normal = 0,     //��������
    em_mode_wait_result     //�ȴ����˽��
};

#define  IDC_STATUS                     100077
#define  MSG_UPDATE_ITEM_SPACE          (WM_USER + 10001)
#define  MSG_TIMER_EVENT_ID             (WM_USER + 10011)
#define  MSG_SEARCH_WINDOW              (WM_USER + 10050)

//status��������ʾ��Ϣ
#define MSG_STATUS_VERMSG               (WM_USER + 10051)   //�汾��Ϣ
#define MSG_STATUS_PACKETS_COUNT        (WM_USER + 10052)   //�������
#define MSG_STATUS_SELECTMSG            (WM_USER + 10053)   //ѡ�������Χ
#define MSG_STATUS_VALUE                (WM_USER + 10054)   //ѡ�����ֵ
#define MSG_LOAD_PACKETFILE             (WM_USER + 10055)   //���ط�������ļ�

#define MSG_INTERNAL_RBUTTONUP          (WM_USER + 10071)   //����Ҽ�����wp:���ھ�� lp:����
#define MSG_SET_FILTER                  (WM_USER + 10081)   //���˹�����Ч
#define MSG_FILTER_COMPLETE             (WM_USER + 10083)   //�����������

mstring g_def_dir;
MainViewMode gsViewMode = em_mode_normal;

const char *s_partition_class_name = ("PartitionClass");
HWND g_main_view = NULL;
//��¼���ѡ����б���
static int s_last_select = 0;
static HWND s_toolbar = NULL;
static HWND s_status = NULL;
static HWND s_splitter = NULL;
//�﷨���˴���
static HWND gsEditFilter = NULL;
static HWND gsBtnExp = NULL;;
static HWND gsWndSniff = NULL;

static HWND s_list = NULL;
static int s_coord = 0;
static int s_status_high = 0;
static int s_splitter_high = 5;
static int s_toolbar_high = 0;
static BOOL g_top_most = FALSE;

//���
static HCURSOR s_cursor_arrow = NULL;
static HCURSOR s_cursor_target = NULL;

//toolbar ���໯
PWIN_PROC s_toolbar_proc = NULL;
const int s_search_idex = 13;
RECT s_search_rect = {0};

//
static BOOL s_window_capter = FALSE;
static HWND s_cur_capter_window = NULL;

//Filter Edit
static DWORD gsRightColour = RGB(175, 255, 175);
static DWORD gsErrorColour = RGB(255, 175, 175);
static HBRUSH gsBrushRight = NULL;
static HBRUSH gsBrushError = NULL;
static HHOOK gsKeyboardHook = NULL;
static DWORD gsEditMode = 0;    //0:righit 1:faild
static CProgressDlg *gsProgressDlg = NULL;

//dlg�ؼ����ڻص�
typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);
static PWIN_PROC g_list_proc = NULL;
static PWIN_PROC g_hex_proc = NULL;

struct MainViewMsg
{
    HWND m_hwnd;
    WPARAM m_wp;
    LPARAM m_lp;
};

//�û���ѡ��ı��ˣ�����״̬��
struct USER_SELECT_MSG
{
    ULONG m_select_begin;   //ѡ����ʼλ��
    ULONG m_select_end;     //ѡ�����λ��
    ULONG m_hlight_begin;   //������ʼλ��
    ULONG m_hlight_end;     //��������λ��
};

static HANDLE s_msg_lock = CreateMutexA(NULL, FALSE, NULL);
#define LOCKMSG            (WaitForSingleObject(s_msg_lock, INFINITE))
#define UNLOCKMSG        (ReleaseMutex(s_msg_lock))
static map<DWORD, MainViewMsg> s_msgs;
static UINT_PTR s_timer = 0;

static HANDLE s_state_lock = CreateMutexA(NULL, FALSE, NULL);
#define  LOCK_STATE     WaitForSingleObject(s_state_lock, INFINITE)
#define  UNLOCK_STATE   ReleaseMutex(s_state_lock)
static map<int, mstring> s_state_msg;

//���Ʊ߿�Ļ���
HPEN s_spy_pen = NULL;

//hex�ؼ�
HWND s_hex = NULL;

#define POPU_MENU_ITEM_CONFIG_MANE                 ("��������   Ctrl+G")
#define POPU_MENU_ITEM_CONFIG_ID                   IDM_CONFIG

#define POPU_MENU_ITEM_COPY_NAME                   ("��������   Ctrl+C")
#define POPU_MENU_ITEM_COPY_ID                     ID_COPY

#define POPU_MENU_ITEM_STREAM_NAME                 ("׷����      Ctrl+T")
#define POPU_MENU_ITEM_STREAM_ID                   ID_STREAM

#define    POPU_MENU_ITEM_SUSPEND_NAME             ("��ͣ��̽   Ctrl+S")
#define    POPU_MENU_ITEM_SUSPEND_ID               ID_SUSPEND

#define    POPU_MENU_ITEM_RESET_NAME               ("�ָ���̽   Ctrl+R")
#define    POPU_MENU_ITEM_RESET_ID                 ID_SUSPEND

#define    POPU_MENU_ITEM_NETCARD_CONFIG_NAME      ("��������   Ctrl+N")
#define    POPU_MENU_ITEM_NETCARD_CONFIG_ID        ID_NET

#define    POPU_MENU_ITEM_POPUP_NAME               ("�����ö�   Ctrl+P")
#define    POPU_MENU_ITEM_POPUP_ID                 ID_TOPMOST

#define    POPU_MENU_ITEM_NOPOPUP_NAME             ("ȡ���ö�   Ctrl+P")
#define    POPU_MENU_ITEM_NOPOPUP_ID               0x0000ff06

#define    POPU_MENU_ITEM_EXPORT_NAME              ("����   Ctrl+O")
#define    POPU_MENU_ITEM_EXPORT_ID                ID_EXPORT

#define    POPU_MENU_ITEM_IMPORT_NAME              ("����   Ctrl+I")
#define    POPU_MENU_ITEM_IMPORT_ID                ID_IMPORT

#define    POPU_MENU_ITEM_CLEAR_NAME               ("�������   Ctrl+X")
#define    POPU_MENU_ITEM_CLEAR_ID                 ID_CLEAR

#define POPU_MENU_ITEM_ABOUT_NAME                  ("����   Ctrl+A")
#define    POPU_MENU_ITEM_ABOUT_ID                 ID_S_ABOUT

#define POPU_MENU_ITEM_HELP_NAME                   ("�����͸���   Ctrl+V")
#define    POPU_MENU_ITEM_HELP_ID                  ID_S_HELP

LRESULT CALLBACK PartitionProc(HWND hdlg, UINT message, WPARAM wp, LPARAM lp);

BOOL WINAPI RegistPartlClass()
{
    WNDCLASSA wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
    wc.hCursor = LoadCursor(NULL, IDC_SIZENS);
    wc.hIcon = NULL;
    wc.hInstance = g_m;
    wc.lpfnWndProc = (WNDPROC)PartitionProc;
    wc.lpszClassName = s_partition_class_name;
    wc.lpszMenuName = NULL;
    wc.style = 0;
    return RegisterClassA(&wc);
}

//����״̬����Ϣ��������,�÷�������printf
VOID WINAPI SetStateMsg(int itm, const char *msg)
{
    LOCK_STATE;
    s_state_msg[itm] = msg;
    UNLOCK_STATE;
    if (0 == itm)
    {
        PostDelayMessage(g_main_view, MSG_STATUS_VERMSG, itm, 0);
    }
    else if (1 == itm)
    {
        PostDelayMessage(g_main_view, MSG_STATUS_PACKETS_COUNT, itm, 0);
    }
    else if (2 == itm)
    {
        PostDelayMessage(g_main_view, MSG_STATUS_SELECTMSG, itm, 0);
    }
    else if (3 == itm)
    {
        PostDelayMessage(g_main_view, MSG_STATUS_VALUE, itm, 0);
    }
}

LRESULT CALLBACK PartitionProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static bool s_catch = false;
    static HWND s_parent = NULL;
    switch(msg)
    {
    case  WM_CREATE:
        {
            s_parent = GetParent(hwnd);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            if (!s_parent)
            {
                break;
            }
            SetCapture(hwnd);
            HDC dc = GetDC(s_parent);
            HPEN pen = CreatePen(PS_SOLID, 2, 0);
            HPEN old_pen = (HPEN)SelectObject(dc, (HGDIOBJ)pen);
            SetROP2(dc, R2_NOTXORPEN);
            RECT rt;
            GetWindowRect(hwnd, &rt);
            MapWindowPoints(NULL, s_parent, (LPPOINT)&rt, sizeof(rt) / sizeof(POINT));
            MoveToEx(dc, rt.left, rt.top, NULL);
            LineTo(dc, rt.right, rt.top);
            SelectObject(dc, old_pen);
            DeleteObject(pen);
            ReleaseDC(hwnd, dc);
            s_coord = rt.top;
            s_catch = true;
        }
        break;
    case  WM_LBUTTONUP:
        {
            if (s_catch)
            {
                RECT rt;
                GetClientRect(s_parent, &rt);
                RECT st;
                GetWindowRect(hwnd, &st);
                MapWindowPoints(NULL, hwnd, (LPPOINT)&st, sizeof(RECT) / sizeof(POINT));
                HDC dc = GetDC(s_parent);
                HPEN pen = CreatePen(PS_SOLID, 2, 0);
                HPEN old_pen = (HPEN)SelectObject(dc, (HGDIOBJ)pen);
                SetROP2(dc, R2_NOTXORPEN);
                MoveToEx(dc, st.left, s_coord, NULL);
                LineTo(dc, st.right, s_coord);
                SelectObject(dc, old_pen);
                DeleteObject(pen);
                ReleaseDC(hwnd, dc);
                ReleaseCapture();
                s_catch = false;
                SendMessageA(s_parent, MSG_UPDATE_ITEM_SPACE, 0, 0);
            }
        }
        break;
    case  WM_MOUSEMOVE:
        {
            if (s_catch && (wp& MK_LBUTTON) == MK_LBUTTON && GetCapture() == hwnd)
            {
                RECT rt;
                GetWindowRect(hwnd, &rt);
                MapWindowPoints(NULL, s_parent, (LPPOINT)&rt, sizeof(rt) / sizeof(POINT));
                HDC dc = GetDC(s_parent);
                HPEN pen = CreatePen(PS_SOLID, 2, 0);
                HPEN old_pen = (HPEN)SelectObject(dc, (HGDIOBJ)pen);
                SetROP2(dc, R2_NOTXORPEN);
                MoveToEx(dc, rt.left, s_coord, NULL);
                LineTo(dc, rt.right, s_coord);
                s_coord = rt.top + (short)HIWORD(lp);
                /*
                if (s_coord <= s_toolbar_high)
                {
                    s_coord = s_toolbar_high + 5;;
                }
                */
                RECT rtList = {0};
                GetWindowRect(s_list, &rtList);
                MapWindowPoints(NULL, s_parent, (LPPOINT)&rtList, sizeof(rtList) / sizeof(POINT));

                if (s_coord <= rtList.top)
                {
                    s_coord = rtList.top + 5;
                }

                RECT ct;
                RECT cl;
                GetClientRect(s_parent, &cl);
                GetWindowRect(s_status, &ct);
                MapWindowPoints(NULL, s_parent, (LPPOINT)&ct, sizeof(ct) / sizeof(POINT));
                long bt = cl.bottom - (ct.bottom - ct.top);
                if (s_coord >= bt)
                {
                    s_coord = bt - 5;
                }
                MoveToEx(dc, rt.left, s_coord, NULL);
                LineTo(dc, rt.right, s_coord);
                SelectObject(dc, (HGDIOBJ)old_pen);
                DeleteObject(pen);
                ReleaseDC(hwnd, dc);
            }
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

VOID AdustWindowItem()
{
    RECT tts;
    RECT rts;
    RECT rtk;
    GetWindowRect(s_toolbar, &tts);
    GetWindowRect(s_list, &rts);
    GetWindowRect(s_hex, &rtk);
    float hs = float(rts.bottom - rts.top) / (float(rtk.bottom - rtk.top) + float(rts.bottom - rts.top));
    float hk = 1.0f - hs;
    CTL_PARAMS arry[] =
    {
        //{0, s_toolbar, 0, 0, 1.0f, 0},
        {NULL, gsEditFilter, 0, 0, 1.0f, 0},
        {NULL, gsBtnExp, 1, 0, 0, 0},
        {NULL, gsWndSniff, 1, 0, 0, 0},
        {IDC_PACKET_LIST, NULL, 0, 0, 1.0f, hs},
        {NULL, s_hex, 0, hs, 1.0f, hk},
        {0, s_splitter, 0, hs, 1, 0},
        {0, s_status, 0, 1, 1, 0}
    };
    SetCtlsCoord(g_main_view, arry, sizeof(arry) / sizeof(CTL_PARAMS));
    return;
}

VOID WINAPI InitListView()
{
    ListView_SetExtendedListViewStyle(s_list, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    LVCOLUMNA col;
    memset(&col, 0x00, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 40;
    col.pszText = (LPSTR)"#";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);

    col.cx = 180;
    col.pszText = (LPSTR)"ʱ��";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 1, (LPARAM)&col);

    col.cx = 80;
    col.pszText = (LPSTR)"Э��";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 2, (LPARAM)&col);

    col.cx = 120;
    col.pszText = (LPSTR)"Դ��ַ";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 3, (LPARAM)&col);

    col.cx = 80;
    col.pszText = (LPSTR)"Դ�˿�";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 4, (LPARAM)&col);
    
    col.cx = 120;
    col.pszText = (LPSTR)"Ŀ���ַ";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 5, (LPARAM)&col);

    col.cx = 80;
    col.pszText = (LPSTR)"Ŀ��˿�";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 6, (LPARAM)&col);

    col.cx = 50;
    col.pszText = (LPSTR)"����";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 7, (LPARAM)&col);

    col.cx = 300;
    col.pszText = (LPSTR)"����";
    SendMessageA(s_list, LVM_INSERTCOLUMNA, 8, (LPARAM)&col);
}

HWND WINAPI CreateStatusBar(HWND hdlg)
{
    HWND status = CreateStatusWindowA(WS_CHILD | WS_VISIBLE, NULL, hdlg, IDC_STATUS);
    int wide[5] = {0};
    int length = 0;
    //����
    wide[0] = 280;
    //���ͳ��
    wide[1] = wide[0] + 360;
    //ѡ��Χ
    wide[2]= wide[1] + 160;
    //ѡ�����ֵ
    wide[3] = wide[2] + 360;
    //���õ�
    wide[4] = wide[3] + 256;
    SendMessage(status, SB_SETPARTS, sizeof(wide) / sizeof(int), (LPARAM)(LPINT)wide); 
    return status;
}

LRESULT CALLBACK ListCtrlProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (hwnd == s_list)
    {
        if (WM_KEYDOWN == msg)
        {
            SendMessageA(g_main_view, WM_KEYDOWN, wp, 0);
        }

        if (WM_KEYUP ==  msg)
        {
            SendMessageA(g_main_view, WM_KEYUP, wp, 0);
        }

        if (WM_RBUTTONUP == msg)
        {
            SendMessageA(g_main_view, MSG_INTERNAL_RBUTTONUP, (WPARAM)s_list, 0);
        }
    }
    return CallWindowProc(g_list_proc, hwnd, msg, wp, lp);
}

LRESULT CALLBACK HexCtrlProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (hwnd == s_hex)
    {
        if (WM_KEYDOWN == msg)
        {
            SendMessageA(g_main_view, WM_KEYDOWN, wp, 0);
        }

        if (WM_KEYUP ==  msg)
        {
            SendMessageA(g_main_view, WM_KEYUP, wp, 0);
        }

        if (WM_RBUTTONUP == msg)
        {
            SendMessageA(g_main_view, MSG_INTERNAL_RBUTTONUP, (WPARAM)s_hex, 0);
        }
    }
    return CallWindowProc(g_hex_proc, hwnd, msg, wp, lp);
}

VOID OnAnalysisDumpFile(const char *file)
{
    mstring vv = "SnifferView - ���ݷ���";
    mstring name;
    name.format("%hs\\\\%hs", vv.c_str(), file);
    HWND hwd = FindWindowA(NULL, name.c_str());
    if (hwd)
    {
        SendMessageA(hwd, MSG_ACTIVE_WINDOW, 0, 0);
        ExitProcess(0);
    }

    vv.format("SnifferView - ���ݷ���\\\\���ڷ��� %hs", file);
    SetWindowTextA(g_main_view, vv.c_str());
    if(DumpPacketsFromFile(file))
    {
        vv.format("SnifferView - ���ݷ���\\\\%hs", file);
        SetWindowTextA(g_main_view, vv.c_str());
        RecheckShowPacket();
        PostMessageA(g_main_view, MSG_UPDATE_DATA, 0, 0);
        g_sniffer_file = file;
        g_analysis_state = TRUE;
    }
    else
    {
        vv.format("SnifferView - ���ݷ���\\\\���� %hs ʧ��", file);
        g_sniffer_file.clear();
        g_analysis_state = FALSE;
    }
    SetWindowTextA(g_main_view, vv.c_str());
}

VOID RecheckFileRelation()
{
    if (CheckFileRelation(SNIFFER_DATA_EXT, SNIFFER_DATA_KEY))
    {
        mstring sub = SNIFFER_DATA_KEY;
        sub += "\\Shell\\Open\\Command";
        mstring cmd;
        DWORD length = MAX_PATH;
        SHGetValueA(HKEY_CLASSES_ROOT, sub.c_str(), "", NULL, cmd.alloc(MAX_PATH), &length);
        cmd.setbuffer();
        int m = cmd.find(' ');
        BOOL relink = FALSE;
        if (mstring::npos == m)
        {
            relink = TRUE;
        }
        else
        {
            cmd.erase(m, cmd.size() - m);
            if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(cmd.c_str()))
            {
                relink = TRUE;
            }
        }

        if (relink)
        {
            //���pe�ļ�·���ı��ˣ����½���ע��
            mstring mv;
            GetModuleFileNameA(NULL, mv.alloc(MAX_PATH), MAX_PATH);
            mv.setbuffer();
            mstring ico(mv.c_str());
            mv += " /f";
            ico += ",1";
            RegisterFileRelation(SNIFFER_DATA_EXT, mv.c_str(), SNIFFER_DATA_KEY, ico.c_str(), SNIFFER_DATA_DEC);
        }
    }
}

static void _InitSnifWatcher()
{
    //ע��۲����ӿ�,Ҫע���Ⱥ�˳��˳���ܴ�
    RegistPacketWatcher(CPacketUnique::PacketInit);
    RegistPacketWatcher(CPacketCacheMgr::PacketAttrInit);
    RegistPacketWatcher(HttpWatcher);
    BeginWork();
    InitSnifferServers();
}

VOID OnSnifferExit()
{
    ClearWatchers();
    StopAllSnifferServers();
    EndWork();
}

PWIN_PROC g_tool_bar = NULL;

HWND CreateToolBar(HWND parent)
{
    int item = 0;
    TBBUTTON tbb_sniffer[] =
    {
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {1, ID_IMPORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},
        {2, ID_EXPORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {11, ID_SUSPEND, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��ͣ��̽")},
        {5, ID_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("�������")},
        {6, ID_TOPMOST, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("�����ö�")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {7, ID_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},
        {8, ID_NET, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {14, ID_NETSTAT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("����״̬")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {13, ID_WINFIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("���Ҵ���")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {10, ID_S_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("���˹������")},
    };

    TBBUTTON tbb_watcher[] =
    {
        //{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        //{1, ID_IMPORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},
        //{2, ID_EXPORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        //{11, ID_SUSPEND, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��ͣ��̽")},
        //{5, ID_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("�������")},
        {6, ID_TOPMOST, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("�����ö�")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {7, ID_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},
        //{8, ID_NET, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("��������")},

        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

        {10, ID_S_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, (INT_PTR)("���˹������")},
    };

    HIMAGELIST image = NULL;
    HBITMAP hBitmapHot = NULL;
    HBITMAP disable = NULL;
    HWND toolbar = NULL;
    do 
    {
        image = ImageList_Create(16, 15, ILC_COLOR32 | ILC_MASK, 2, 2);
        if (!image)
        {
            break;
        }
        hBitmapHot = LoadBitmap(g_m, MAKEINTRESOURCE(IDB_TOOLBAR));
        //disable = LoadBitmap(g_m, MAKEINTRESOURCE(IDB_DISABLE));
        if(!hBitmapHot)
        {
            break;
        }
        toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,  WS_CHILD | TBSTYLE_FLAT | WS_BORDER | CCS_NOMOVEY | CCS_ADJUSTABLE | TBSTYLE_TOOLTIPS | TBSTYLE_ALTDRAG , 0, 0, 0, 0, parent,  NULL, g_m, NULL); 
        ImageList_AddMasked(image, hBitmapHot, 0xc0c0c0);
        //ImageList_AddMasked(image, disable, 0xc0c0c0);
        SendMessage(toolbar, TB_SETIMAGELIST, 0, (LPARAM)image);
        SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
        if (g_work_state == em_sniffer)
        {
            SendMessage(toolbar, TB_ADDBUTTONS, (WPARAM)(sizeof(tbb_sniffer) / sizeof(TBBUTTON)), (LPARAM)(LPTBBUTTON)&tbb_sniffer);
        }
        else
        {
            SendMessage(toolbar, TB_ADDBUTTONS, (WPARAM)(sizeof(tbb_watcher) / sizeof(TBBUTTON)), (LPARAM)(LPTBBUTTON)&tbb_watcher);
        }
        SendMessage(toolbar, TB_SETMAXTEXTROWS, (WPARAM) 0, 0);;
        ShowWindow(toolbar, SW_SHOW);
    } while (FALSE);

    if(hBitmapHot)
    {
        DeleteObject(hBitmapHot);
    }

    //if (disable)
    //{
    //    DeleteObject(disable);
    //}
    return toolbar;
}

//��ʼ�����ڿؼ���λ�ã�������������Ϊtoolbar, filterEdit, listctrl, splitter, hex, status
VOID InitWindowPos()
{
    int pos = 0;
    //main
    RECT client_rect;
    GetClientRect(g_main_view, &client_rect);
    int c_width = client_rect.right - client_rect.left;
    int c_high = client_rect.bottom - client_rect.top;

    //filterEdit, ExpButton, bmpCtrl
    RECT filterRect = {0};
    GetWindowRect(gsEditFilter, &filterRect);
    int filterWidth = filterRect.right - filterRect.left;
    int filterHigh = filterRect.bottom - filterRect.top;
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&filterRect, sizeof(filterRect) / sizeof(POINT));
    MoveWindow(gsEditFilter, client_rect.left, client_rect.top + pos, c_width - 160, filterHigh, TRUE);

    //button, image
    RECT rt1, rt2;
    GetWindowRect(gsEditFilter, &rt1);
    GetWindowRect(gsBtnExp, &rt2);
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&rt1, 2);
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&rt2, 2);
    SetWindowPos(gsBtnExp, 0, rt1.right + 10, rt1.top, rt2.right - rt2.left, rt1.bottom - rt1.top, SWP_NOZORDER);
    GetWindowRect(gsBtnExp, &rt2);
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&rt2, 2);
    SetWindowPos(gsWndSniff, 0, rt2.right + 10, rt2.top + 5, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    pos += (filterHigh + 5);

    //listctrl
    RECT list_rect;
    GetWindowRect(s_list, &list_rect);
    int l_width = list_rect.right - list_rect.left;
    int l_high = list_rect.bottom - list_rect.top;
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&list_rect, sizeof(list_rect) / sizeof(POINT));
    MoveWindow(s_list, client_rect.left, client_rect.top + pos - 1, c_width, l_high + 1, TRUE);
    pos += l_high;

    //splitter
    MoveWindow(s_splitter, client_rect.left, pos, c_width, s_splitter_high, TRUE);
    pos += s_splitter_high;

    RECT s_rect;
    GetWindowRect(s_status, &s_rect);
    MapWindowPoints(NULL, g_main_view, (LPPOINT)&s_rect, sizeof(s_rect) / sizeof(POINT));
    int s_width = s_rect.right - s_rect.left;
    int s_high = s_rect.bottom - s_rect.top;
    s_status_high = s_high;

    //��Ϊstatus�ؼ��ĸ߶��ǹ̶��ģ��Ȼ�ȡstatus�ĸ߶ȣ�����status�ĸ߶ȵ���hex�ĸ߶�
    //�߶�����һ�����أ���Ϊ��server2008���������п�϶
    int h_high = c_high - pos - s_status_high;
    MoveWindow(s_hex, client_rect.left, pos, c_width, h_high + 1, TRUE);
    pos += h_high;

    //status
    MoveWindow(s_status, client_rect.left, pos, c_width, s_status_high, TRUE);
}

//��ʼ�����ڲ˵�
VOID InitWindowMenu()
{
    if (g_work_state == em_analysis)
    {
        HMENU menu = GetMenu(g_main_view);
        HMENU sub = GetSubMenu(menu, 0);
        DeleteMenu(menu, ID_IMPORT, MF_BYCOMMAND);
        DeleteMenu(menu, ID_EXPORT, MF_BYCOMMAND);

        sub = GetSubMenu(menu, 1);
        DeleteMenu(menu, ID_SUSPEND, MF_BYCOMMAND);
        DeleteMenu(menu, ID_CLEAR, MF_BYCOMMAND);

        sub = GetSubMenu(menu, 2);
        DeleteMenu(menu, ID_FILTER, MF_BYCOMMAND);
        DeleteMenu(menu, ID_NET, MF_BYCOMMAND);
    }
}

LRESULT CALLBACK ToolbarCtrlProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (hwnd == s_toolbar)
    {
        if (WM_LBUTTONDOWN == msg)
        {
            int x = LOWORD(lp);
            int y = HIWORD(lp);
            if (x > s_search_rect.left && x < s_search_rect.right && y > s_search_rect.top && y < s_search_rect.bottom)
            {
                //
                PostMessageA(g_main_view, MSG_SEARCH_WINDOW, 0, 0);
            }
        }
    }
    return CallWindowProc(s_toolbar_proc, hwnd, msg, wp, lp);
}

static BOOL _ChangeWndMessageFilter(UINT uMessage, BOOL bAllow)
{
#define MSGFLT_ADD      1
#define MSGFLT_REMOVE   2
    HMODULE hUserMod = NULL;
    BOOL bResult = FALSE;
    typedef BOOL (WINAPI* ChangeWindowMessageFilterFn)(UINT, DWORD);

    hUserMod = LoadLibraryW(L"user32.dll");
    if (hUserMod == NULL)
    {
        return FALSE;
    }

    ChangeWindowMessageFilterFn pfnChangeWindowMessageFilter = 
        (ChangeWindowMessageFilterFn)GetProcAddress(hUserMod, "ChangeWindowMessageFilter");
    if (pfnChangeWindowMessageFilter == NULL)
    {
        FreeLibrary(hUserMod);
        return FALSE;
    }

    bResult = pfnChangeWindowMessageFilter(uMessage, bAllow ? MSGFLT_ADD : MSGFLT_REMOVE);
    FreeLibrary(hUserMod);
    return bResult;
}

static void _InitSniffer() {
    mstring dllPath;
    dllPath = gInstallPath;

    dllPath.path_append("SyntaxView.dll");
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(dllPath.c_str()))
    {
        ReleaseRes(dllPath.c_str(), IDR_DLL_SYNTAX, "DLL");
    }
    LoadLibraryA(dllPath.c_str());
}

//Filter Edit Init
static void _InitFilterEdit() {
    gsBrushRight = CreateSolidBrush(gsRightColour);
    gsBrushError = CreateSolidBrush(gsErrorColour);

    LOGFONTA lf = {0}; 
    lf.lfHeight = 16;
    lstrcpyA(lf.lfFaceName, "Courier New");

    HFONT hFont = CreateFontIndirectA(&lf);
    SendMessageA(gsEditFilter, WM_SETFONT, (WPARAM)hFont, TRUE);
    SetFocus(gsEditFilter);
    SetWindowTextA(gsEditFilter, g_show_string.c_str());

    int len = GetWindowTextLengthA(gsEditFilter);
    SendMessageA(gsEditFilter, EM_SETSEL, len, len);
}

static HANDLE gsRuleCheckNotify = CreateEventA(NULL, FALSE, FALSE, NULL);
static DWORD WINAPI _FilterRuleThread(LPVOID param) {
    while (true) {
        WaitForSingleObject(gsRuleCheckNotify, 500);

        static mstring sStrLastRule;
        mstring str = GetWindowStrA(gsEditFilter);
        str.trim();

        if (sStrLastRule != str)
        {
            if (str.empty())
            {
                gsEditMode = 0;
            } else {
                list<FilterRules> showRules;
                if (RulesCompile(str.c_str(), showRules))
                {
                    gsEditMode = 0;
                } else {
                    gsEditMode = 1;
                }
            }
            sStrLastRule = str;
            InvalidateRect(gsEditFilter, NULL, TRUE);
        }
    }
}

static void _NoitfyFilterCheck() {
    SetEvent(gsRuleCheckNotify);
}

static LRESULT CALLBACK _KeyCaptureProc(int code, WPARAM wp, LPARAM lp) {
    if (code < 0)
    {
        return CallNextHookEx(gsKeyboardHook, code, wp, lp);
    }

    if (GetFocus() == gsEditFilter)
    {
        _NoitfyFilterCheck();
        //dp(L"key:%d", wp);

        if (wp == '\r')
        {
            SendMessageA(g_main_view, MSG_SET_FILTER, 0, 0);
        }
    }

    //����������Ҫ�����������̲���
    if (GetAsyncKeyState(VK_CONTROL) & (1 << 16))
    {
        int repeat = (lp & 0x100);
        dp(L"ctrl test:%d, key:%d", repeat, wp);

        DWORD command = 0;
        switch (wp) {
            case 'c':
                break;
            default:
                break;
        }
    }
    return CallNextHookEx(gsKeyboardHook, code, wp, lp);
}

VOID OnInitDialog(HWND hdlg)
{
    g_main_view = hdlg;
    s_spy_pen = (HPEN)GetStockObject(WHITE_PEN);
    SendMessageA(hdlg, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconA(g_m, MAKEINTRESOURCEA(IDI_MAIN)));
    s_status = CreateStatusBar(hdlg);
    gsWndSniff = GetDlgItem(hdlg, IDC_CTRL_WND_SNIFF);
    RegistPartlClass();
    gsProgressDlg = new CProgressDlg();

    gsEditFilter = GetDlgItem(hdlg, IDC_EDT_FILTER);
    _InitFilterEdit();
    gsKeyboardHook = SetWindowsHookExA(WH_KEYBOARD, _KeyCaptureProc, NULL, GetCurrentThreadId());
    CloseHandle(CreateThread(NULL, 0, _FilterRuleThread, NULL, 0, NULL));

    gsBtnExp = GetDlgItem(hdlg, IDC_BTN_EXP);
    s_splitter = CreateWindowExA(NULL, s_partition_class_name, "", WS_VISIBLE | WS_CHILD, 10, 0, 0, 0, hdlg, NULL, g_m, NULL);
    s_list = GetDlgItem(hdlg, IDC_PACKET_LIST);
    s_hex = CreateHexView(hdlg, 0, 0);
    InitListView(); 

    g_list_proc = (PWIN_PROC)SetWindowLong(s_list, GWL_WNDPROC, (long)ListCtrlProc);
    g_hex_proc = (PWIN_PROC)SetWindowLong(s_hex, GWL_WNDPROC, (long)HexCtrlProc);
 
    s_cursor_arrow = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_ARROW));

    HBITMAP bmp = LoadBitmapA(g_m, MAKEINTRESOURCEA(IDB_TARGET_CUR));
    s_cursor_target = CreateCursorFromBitmap(bmp, RGB(240, 240, 240));
    DeleteObject((HGDIOBJ)bmp);
    
    mstring vv;
    GetModuleFileNameA(NULL, vv.alloc(MAX_PATH), MAX_PATH);
    vv.setbuffer();
    mstring mv;
    GetFileVersion(vv.c_str(), mv);
    vv.format("SnifferView  %hs   %hs", mv.c_str(), "by  lougdhr@126.com");
    SetStateMsg(0, vv.c_str()); 
    //UpdateHexSelect(0, 0, 0, 0);
    SendUserSelectMessage(-1, -1, -1, -1);
    s_timer = SetTimer(hdlg, MSG_TIMER_EVENT_ID, 100, NULL);
    //s_toolbar = CreateToolBar(hdlg);
    SendMessageA(s_toolbar, TB_GETITEMRECT, s_search_idex, (LPARAM)&s_search_rect);
    if (g_work_state == em_sniffer)
    {
        s_toolbar_proc = (PWIN_PROC)SetWindowLong(s_toolbar, GWL_WNDPROC, (long)ToolbarCtrlProc);
    }

    SetWindowPos(hdlg, NULL, 0, 0, 1100, 600, SWP_NOMOVE | SWP_NOZORDER);
    CentreWindow(NULL, hdlg);
    //��ʼ�����ڲ˵�
    InitWindowMenu();
    //��ʼ�����ڿؼ���λ��
    InitWindowPos();
    AdustWindowItem();

    if (g_work_state == em_sniffer)
    {
        CFileCache::GetInst()->InitFileCache();
        CPacketCacheMgr::GetInst()->InitCacheMgr();
        _InitSnifWatcher();
        SetWindowTextA(hdlg, SNIFFER_STATE_NAME);
    }
    else if(g_work_state == em_analysis)
    {
        if (g_sniffer_file.size() != 0)
        {
            OnAnalysisDumpFile(g_sniffer_file.c_str());
        }
    }
    //���ע�����ļ��������м�飬���pe�ļ��ƶ������¹���
    RecheckFileRelation();
    SendMessage(hdlg, MSG_ACTIVE_WINDOW, 0, 0);

    _ChangeWndMessageFilter(WM_DROPFILES, TRUE);
    _ChangeWndMessageFilter(0x0049, TRUE);
    _InitSniffer();
}

VOID WINAPI OnUpdateMsg(HWND hdlg)
{
    long itm = 0;
    RECT rt;
    GetWindowRect(s_list, &rt);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rt, sizeof(rt) / sizeof(POINT));
    MoveWindow(s_list, rt.left, rt.top, rt.right - rt.left, s_coord - rt.top, TRUE);
    itm += (s_coord - rt.top);

    RECT rs;
    GetWindowRect(s_splitter, &rs);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rs, sizeof(rs) / sizeof(POINT));
    MoveWindow(s_splitter, rs.left, s_coord, rs.right - rs.left, s_splitter_high, TRUE);
    itm += s_splitter_high;

    GetWindowRect(s_hex, &rt);
    MapWindowPoints(NULL, hdlg, (LPPOINT)&rt, sizeof(RECT) / sizeof(POINT));

    RECT rk;
    GetClientRect(hdlg, &rk);

    /*
    itm += s_toolbar_high;
    long top = itm - 1;
    long h = rk.bottom - rk.top - itm - s_status_high + 2;
    MoveWindow(s_hex, rk.left, top, rk.right - rk.left, h, TRUE);
    */
    MoveWindow(s_hex, rk.left, s_coord + s_splitter_high, rk.right - rk.left, rt.bottom - (s_coord + s_splitter_high), TRUE);
    AdustWindowItem();
}

VOID WINAPI OnListViewRClick(HWND hdlg, WPARAM wp, LPARAM lp)
{
    POINT pt = {0}; 
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_CONFIG_ID, POPU_MENU_ITEM_CONFIG_MANE);
    AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_COPY_ID, POPU_MENU_ITEM_COPY_NAME);

    bool enableStream = false;
    if (s_last_select >= 0 && (CFileCache::GetInst()->GetShowCount() > (size_t)s_last_select))
    {
        PacketContent pp;
        CFileCache::GetInst()->GetShow(s_last_select, pp);
        if (pp.m_tls_type == em_tls_tcp)
        {
            enableStream = true;
        }
    }

    if (enableStream)
    {
        AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_STREAM_ID, POPU_MENU_ITEM_STREAM_NAME);
    } else {
        AppendMenuA(menu, MF_DISABLED, POPU_MENU_ITEM_STREAM_ID, POPU_MENU_ITEM_STREAM_NAME);
    }

    if (g_work_state == em_sniffer)
    {
        AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_NETCARD_CONFIG_ID, POPU_MENU_ITEM_NETCARD_CONFIG_NAME);
    }

    AppendMenuA(menu, MF_MENUBARBREAK, 0, 0);
    if (g_work_state == em_sniffer)
    {
        if (IsSnifferSuspend())
        {
            AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_RESET_ID, POPU_MENU_ITEM_RESET_NAME);
        }
        else
        {
            AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_SUSPEND_ID, POPU_MENU_ITEM_SUSPEND_NAME);
        }
    }

    if(g_top_most)
    {
        AppendMenuA(menu, MF_ENABLED | MF_CHECKED, POPU_MENU_ITEM_POPUP_ID, POPU_MENU_ITEM_POPUP_NAME);
    }
    else
    {
        AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_POPUP_ID, POPU_MENU_ITEM_POPUP_NAME);
    }

    if (g_work_state == em_sniffer)
    {
        AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_CLEAR_ID, POPU_MENU_ITEM_CLEAR_NAME);
        AppendMenuA(menu, MF_MENUBARBREAK, 0, 0);
        AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_EXPORT_ID, POPU_MENU_ITEM_EXPORT_NAME);
    }
    AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_IMPORT_ID, POPU_MENU_ITEM_IMPORT_NAME);
    AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_ABOUT_ID, POPU_MENU_ITEM_ABOUT_NAME);
    AppendMenuA(menu, MF_ENABLED, POPU_MENU_ITEM_HELP_ID, POPU_MENU_ITEM_HELP_NAME);
    TrackPopupMenu(menu, TPM_CENTERALIGN, pt.x, pt.y, 0, hdlg, 0);
    DestroyMenu(menu);
}

static VOID WINAPI ShowPacketData(int itm)
{
    if (itm < 0)
    {
        SetData(NULL, 0);
        return;
    }

    if (itm < (int)CFileCache::GetInst()->GetShowCount())
    {
        int hight_begin = 0;
        int hight_end = 0;
        int mark = 0;

        PacketContent content;
        CFileCache::GetInst()->GetShow(itm, content);
        SetData((const BYTE *)content.m_packet.c_str(), content.m_packet.size());
        if (content.m_tls_type == em_tls_tcp)
        {
            TCPHeader *tcp = (TCPHeader *)(content.m_packet.c_str() + sizeof(IPHeader));
            mark = tcp->get_tcp_header_length();
        }
        else if (content.m_tls_type == em_tls_udp)
        {
            mark = sizeof(UDPHeader);
        }
        else if (content.m_tls_type == em_tls_icmp)
        {
            mark = sizeof(ICMPHeader);
        }

        if (mark == 0)
        {
            return;
        }

        if (g_hex_hight == em_ip_header)
        {
            hight_begin = 0;
            hight_end = sizeof(IPHeader) - 1;
        }
        else if (g_hex_hight == em_tcp_udp_icmp_header)
        {
            hight_begin = sizeof(IPHeader);
            hight_end = hight_begin + mark - 1;
        }
        else if (g_hex_hight == em_user_data)
        {
            hight_begin = sizeof(IPHeader) + mark;
            hight_end = content.m_packet.size() - 1;
        }

        if (hight_begin >= 0 && hight_end >= hight_begin)
        {
            SetHighlighted(hight_begin, hight_end);
        }
    }
    else
    {
        SetData(NULL, 0);
    }
}

BOOL ShowFileSaveDialog(IN HWND hwnd, IN const char *name, IN const char *defdir, OUT mstring &file)
{
    OPENFILENAMEA ofn;  
    ZeroMemory(&ofn, sizeof(ofn));  
    char filename[MAX_PATH] = {0};
    strcpy_s(filename, sizeof(filename), name);
    ofn.lpstrFile = filename;  
    ofn.nMaxFile = MAX_PATH;  
    ofn.lpstrFilter ="Packet File(*.vex)\0*.vex\0\0";  
    ofn.lpstrDefExt = "vex";
    ofn.lpstrTitle = "�����������ļ�";
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    if (defdir && IsDirectoryExist(defdir))
    {
        ofn.lpstrInitialDir = defdir;    
    }
    ofn.FlagsEx = OFN_EX_NOPLACESBAR;  
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = hwnd;
    if (GetSaveFileName(&ofn))  
    {  
        file = filename;
        return TRUE;
    }
    return FALSE;
}

BOOL ShowFileOpenDialog(IN HWND hwnd, IN const char *defdir, OUT mstring &file)
{
    OPENFILENAMEA ofn;  
    ZeroMemory(&ofn, sizeof(ofn));  
    char filename[MAX_PATH] = {0};
    ofn.lpstrFile = filename;  
    ofn.nMaxFile = MAX_PATH;  
    ofn.lpstrFilter ="Packet File(*.vex)\0*.vex\0\0";  
    ofn.lpstrDefExt = "vex";
    ofn.lpstrTitle = "�򿪷�������ļ�";
    ofn.Flags = 0x0008182c;
    if (defdir && IsDirectoryExist(defdir))
    {
        ofn.lpstrInitialDir = defdir;    
    }
    ofn.FlagsEx = OFN_EX_NOPLACESBAR;  
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = hwnd;
    if (GetOpenFileName(&ofn))  
    {  
        file = filename;
        return TRUE;
    }
    return FALSE;
}

VOID WINAPI OnListViewLClick(WPARAM wp, LPARAM lp)
{
    NM_LISTVIEW *listview = (NM_LISTVIEW *)lp;
    int itm = listview->iItem;
    if (-1 == itm)
    {
        return;
    }
    ShowPacketData(itm);
}

static BOOL WINAPI _DressLoggedOnUser()
{
    HANDLE hToken = NULL;
    if (WTSQueryUserToken(WTS_CURRENT_SESSION, &hToken))
    {
        if (ImpersonateLoggedOnUser(hToken))
        {
            return TRUE;
        }
    }
    return FALSE;
}

VOID WINAPI OnExportFile()
{
    mstring file;
    SYSTEMTIME vv;
    GetLocalTime(&vv);
    mstring name;
    name.format("%04d%02d%02d%02d%02d%02d%03d", vv.wYear, vv.wMonth, vv.wDay, vv.wHour, vv.wMinute, vv.wSecond, vv.wMilliseconds);
    //_DressLoggedOnUser();
    do 
    {
        if (ShowFileSaveDialog(g_main_view, name.c_str(), g_def_dir.c_str(), file))
        {
            if (INVALID_FILE_ATTRIBUTES != GetFileAttributesA(file.c_str()))
            {
                mstring msg;
                msg.format("�ļ�%hs�Ѵ��ڣ��Ƿ񸲸ǣ�", file.c_str());
                if (IDOK != MessageBoxA(g_main_view, msg.c_str(), "����", MB_OKCANCEL | MB_ICONWARNING))
                {
                    break;
                }
            }

            BOOL ret = DumpPacketsToFile(file.c_str());

            file.repsub("/", "\\");
            mstring dir = file;
            int m = dir.rfind('\\');
            dir.erase(m, dir.size() - m);
            if (g_def_dir != dir)
            {
                g_def_dir = dir;
                SaveFilterConfig();
            }

            if (ret)
            {
                if (IDOK == MessageBoxA(g_main_view, "�ļ������ɹ����Ƿ�λ���ļ���", "�����ɹ�", MB_OKCANCEL | MB_ICONQUESTION))
                {
                    mstring cmd;
                    cmd.format("%hs,\"%hs\"", "/select", file.c_str());
                    ShellExecuteA(NULL, "open", "explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
                }
            }
        }
    } while (FALSE);
    //RevertToSelf();
}

VOID OnImportFile()
{
    mstring strPath;
    ShowFileOpenDialog(g_main_view, g_def_dir.c_str(), strPath);
    if (!strPath.empty())
    {
        SendMessageA(g_main_view, MSG_LOAD_PACKETFILE, (WPARAM)(strPath.c_str()), NULL);
    }
    return;
}

//����toolbar��Ϣ
VOID SetToolbarInfo(int id, int image, const char *msg)
{
    TBBUTTONINFO   tbinfo; 
    memset(&tbinfo, 0, sizeof(TBBUTTONINFO)); 
    tbinfo.cbSize = sizeof(TBBUTTONINFO); 
    tbinfo.dwMask=TBIF_TEXT | TBIF_IMAGE;
    tbinfo.pszText = (LPSTR)msg;
    tbinfo.iImage = image;
    SendMessage(s_toolbar, TB_SETBUTTONINFO, id, LPARAM(&tbinfo));
}

//��ͣ��̽����
VOID OnSuspendSniffer()
{
    HMENU menu = NULL;
    menu = GetMenu(g_main_view);
    menu = GetSubMenu(menu, 1);
    UINT id = ID_SUSPEND;
    if(IsSnifferSuspend())
    {
        //�ָ���̽����
        RestSniffer();
        SetWindowTextA(g_main_view, SNIFFER_STATE_NAME);
        ModifyMenu(menu, id, MF_BYCOMMAND, id, "��ͣ��̽");
        SetToolbarInfo(ID_SUSPEND, 11, "��ͣ��̽");
    }
    else
    {
        //��ͣ��̽����
        SuspendSniffer();
        SetWindowTextA(g_main_view, SNIFFER_SUSPEND_NAME);
        ModifyMenu(menu, id, MF_BYCOMMAND, id, "�ָ���̽");
        SetToolbarInfo(ID_SUSPEND, 12, "�ָ���̽");
    }
}

VOID OnTopMost()
{
    HMENU menu = NULL;
    menu = GetMenu(g_main_view);
    menu = GetSubMenu(menu, 1);
    DWORD id = ID_TOPMOST;
    if (!g_top_most)
    {
        SetWindowPos(g_main_view, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ModifyMenu(menu, id, MF_BYCOMMAND, id, "ȡ���ö�");
        SetToolbarInfo(ID_TOPMOST, 6, "ȡ���ö�");
    }
    else
    {
        SetWindowPos(g_main_view, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ModifyMenu(menu, id, MF_BYCOMMAND, id, "�����ö�");
        SetToolbarInfo(ID_TOPMOST, 6, "�����ö�");
    }
    g_top_most = !g_top_most;
}

static VOID _OnCopyData(WPARAM wp, LPARAM lp)
{
    mstring strData;
    GetHexSelectData(strData);
    if (strData.empty())
    {
        return;
    }
    HGLOBAL hBuffer = NULL;
    do 
    {
        if (!OpenClipboard(NULL))
        {
            break;
        }
        EmptyClipboard();
        hBuffer = GlobalAlloc(GMEM_MOVEABLE, strData.size() + 16);
        if (!hBuffer)
        {
            break;
        }
        LPSTR pBuffer = (LPSTR)GlobalLock(hBuffer);
        if (!pBuffer)
        {
            break;
        }
        lstrcpynA(pBuffer, strData.c_str(), strData.size() + 16);
        GlobalUnlock(hBuffer);
        SetClipboardData(CF_TEXT, hBuffer);
    } while (FALSE);
    if (hBuffer)
    {
        GlobalFree(hBuffer);
    }
    CloseClipboard();
    return;
}

static void _OnSearchWindow(HWND hdlg, WPARAM wp, LPARAM lp)
{
    ShowWindow(hdlg, SW_HIDE);
    //�����Ϣֻ��ͨ��PostMessage���з��ͣ��������óɹ���ԭ�����
    SetCursor(s_cursor_target);
    SetCapture(g_main_view);
    s_cur_capter_window = NULL;
    s_window_capter = TRUE;
}

VOID WINAPI OnCommand(WPARAM wp, LPARAM lp)
{
    DWORD id = LOWORD(wp);
    switch(id)
    {
    case  POPU_MENU_ITEM_CONFIG_ID:
        {
            ShowConfigView(g_main_view);
        }
        break;
    case  POPU_MENU_ITEM_COPY_ID:
        {
            _OnCopyData(wp, lp);
        }
        break;
    case POPU_MENU_ITEM_STREAM_ID:
        {
            ShowStreamView(g_main_view, s_last_select);
        }
        break;
    case  POPU_MENU_ITEM_SUSPEND_ID:
        {
            OnSuspendSniffer();
        }
        break;
    case  POPU_MENU_ITEM_NETCARD_CONFIG_ID:
        {
            ShowNetConfigView();
        }
        break;
    case  POPU_MENU_ITEM_POPUP_ID:
        {
            OnTopMost();
        }
        break;
    case  POPU_MENU_ITEM_EXPORT_ID:
        {
            OnExportFile();
        }
        break;
    case  POPU_MENU_ITEM_ABOUT_ID:
        {
            ShowAbout();
        }
        break;
    case  POPU_MENU_ITEM_HELP_ID:
        {
            ShowHelpView(g_main_view);
        }
        break;
    case  POPU_MENU_ITEM_IMPORT_ID:
        {
            OnImportFile();
        }
        break;
    case  POPU_MENU_ITEM_CLEAR_ID:
        {
            ClearPackets();
            NoticefSnifferViewRefush();
        }
        break;
    case  ID_NETSTAT:
        {
            RunNetstatView(g_main_view, 0);
        }
        break;
    case IDC_BTN_EXP:
        {
            ShowHelpView(g_main_view);
        }
        break;
    case IDC_CTRL_WND_SNIFF:
        {
            _OnSearchWindow(g_main_view, wp, lp);
        }
        break;
    case  ID_EXIT:
        {
            SendMessage(g_main_view, WM_CLOSE, 0, 0);
        }
        break;
    }
}

VOID WINAPI OnGetListCtrlDisplsy(IN OUT NMLVDISPINFO* plvdi)
{
    int itm = plvdi->item.iItem;
    int sub = plvdi->item.iSubItem;
    int mark = 0;
    if ((int)CFileCache::GetInst()->GetShowCount() <= itm)
    {
        return;
    }

    PacketContent tmp;
    CFileCache::GetInst()->GetShow(itm, tmp);
    //# ʱ�� Э�� Դ��ַ Դ�˿� Զ�˵�ַ Զ�˶˿� ���� ����
    static mstring ts;
    if (0 == sub)
    {
        ts.format("%d", itm);
    }
    else if (1 == sub)
    {
        ts = tmp.m_time;
    }
    else if(2 == sub)
    {
        if (tmp.m_tls_type == em_tls_tcp)
        {
            ts = "Tcp";
        }

        if (tmp.m_tls_type == em_tls_udp)
        {
            ts = "Udp";
        }

        if (tmp.m_tls_type == em_tls_icmp)
        {
            ts = "Icmp";
        }

        if (0 == ts.size())
        {
            ts = "IP";
        }

        if (tmp.m_user_type == em_user_http)
        {
            ts = "Http";
            if (tmp.m_user_content.m_http.m_http_type & em_http_get)
            {
                ts += "\\Get";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_post)
            {
                ts += "\\Post";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_head)
            {
                ts += "\\Head";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_options)
            {
                ts += "\\Options";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_put)
            {
                ts += "\\Put";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_delete)
            {
                ts += "\\Delete";
            }
            else if (tmp.m_user_content.m_http.m_http_type & em_http_tarce)
            {
                ts += "\\Tarce";
            }
            else
            {
                ts += "\\Resp";
            }
        }

        if (mstring::npos != ts.find("2019-"))
        {
            int d = 1234;
        }
    }
    else if (3 == sub)
    {
        DWORD addr = h2n_32(tmp.m_ip_header.m_ipSource);
        IN_ADDR as;
        as.S_un.S_addr = addr;
        ts = inet_ntoa(as);
    }
    else if (4 == sub)
    {
        if (tmp.m_tls_type == em_tls_tcp)
        {
            ts.format("%u", tmp.m_tls_header.m_tcp.m_sourcePort);
        }
        else if (tmp.m_tls_type == em_tls_udp)
        {
            ts.format("%u", tmp.m_tls_header.m_udp.m_sourcePort);
        }
        else
        {
            ts = "";
        }
    }
    else if (5 == sub)
    {
        DWORD addr = h2n_32(tmp.m_ip_header.m_ipDestination);
        IN_ADDR as;
        as.S_un.S_addr = addr;
        ts = inet_ntoa(as);
    }
    else if (6 == sub)
    {
        if (tmp.m_tls_type == em_tls_tcp)
        {
            ts.format("%u", tmp.m_tls_header.m_tcp.m_destinationPort);
        }
        else if (tmp.m_tls_type == em_tls_udp)
        {
            ts.format("%u", tmp.m_tls_header.m_udp.m_destinationPort);
        }
        else
        {
            ts = "";
        }
    }
    else if (7 == sub)
    {
        int length = 0;
        if (tmp.m_tls_type == em_tls_tcp)
        {
            //length = tmp->m_packet.size() - sizeof(IPHeader) - sizeof(TCPHeader);
            TCPHeader *tcp = (TCPHeader *)(tmp.m_packet.c_str() + sizeof(IPHeader));
            length = tmp.m_packet.size() - sizeof(IPHeader) - tcp->get_tcp_header_length();
        }
        else if (tmp.m_tls_type == em_tls_udp)
        {
            length = tmp.m_packet.size() - sizeof(IPHeader) - sizeof(UDPHeader);
        }
        else if (tmp.m_tls_type == em_tls_icmp)
        {
            length = tmp.m_packet.size() - sizeof(IPHeader) - sizeof(ICMPHeader);
        }
        else
        {
            length = tmp.m_packet.size() - sizeof(IPHeader);
        }
        if (length < 0)
        {
            length = 0;
        }
        ts.format("%u", length);
    }
    else if (8 == sub)
    {
        if (0 == tmp.m_show.size())
        {
            GetPacketShowBuffer(&tmp);
        }
        ts = tmp.m_show;
    }

    if (ts.size() > 255)
    {
        ts.at(255) = 0x00;
    }
    plvdi->item.pszText = (LPSTR)(ts.c_str());
}

VOID OnRButtonUp(WPARAM wp, LPARAM lp)
{
}

//Ͷ�ݷ�ʵʱ����Ϣ������Ч��ֹ������Ͷ����ϢƵ�ʹ��ߵ���cpu����
VOID PostDelayMessage(HWND hwnd, DWORD msg, WPARAM wp, LPARAM lp)
{
    LOCKMSG;
    MainViewMsg mv;
    mv.m_hwnd = hwnd;
    mv.m_wp = wp;
    mv.m_lp = lp;
    s_msgs[msg] = mv;
    UNLOCKMSG;
}

VOID OnTimerEvent(WPARAM wp, LPARAM lp)
{
    if (wp != MSG_TIMER_EVENT_ID)
    {
        return;
    }
    LOCKMSG;
    map<DWORD, MainViewMsg>::iterator itm;
    for (itm = s_msgs.begin() ; itm != s_msgs.end() ; itm++)
    {
        if (IsWindow(itm->second.m_hwnd))
        {
            PostMessageA(itm->second.m_hwnd, itm->first, itm->second.m_wp, itm->second.m_lp);
        }
    }
    s_msgs.clear();
    UNLOCKMSG;
}

VOID WINAPI OnKeyDown(WPARAM wp, LPARAM lp)
{
    if (GetAsyncKeyState(VK_CONTROL) & (1 << 16))
    {
        WPARAM param = 0;
        switch(wp)
        {
        case  0x43://ctrl + c
            {
                param = POPU_MENU_ITEM_COPY_ID;
            }
            break;
        case  0x53://ctrl + s
            {
                if (g_work_state == em_sniffer)
                {
                    param = POPU_MENU_ITEM_SUSPEND_ID;
                }
            }
            break;
        case  0x52://ctrl + r
            {
                if (g_work_state == em_sniffer)
                {
                    param = POPU_MENU_ITEM_RESET_ID;
                }
            }
            break;
        case  0x4e://ctrl + n
            {
                if (g_work_state == em_sniffer)
                {
                    param = POPU_MENU_ITEM_NETCARD_CONFIG_ID;
                }
            }
            break;
        case 0x50://ctrl + p
            {
                param = POPU_MENU_ITEM_POPUP_ID;
            }
            break;
        case  0x4f://ctrl + o
            {
                if (g_work_state == em_sniffer)
                {
                    param = POPU_MENU_ITEM_EXPORT_ID;
                }
            }
            break;
        case  0x49://ctrl + i
            {
                param = POPU_MENU_ITEM_IMPORT_ID;
            }
            break;
        case  0x58://ctrl + x
            {
                if (g_work_state == em_sniffer)
                {
                    param = POPU_MENU_ITEM_CLEAR_ID;
                }
            }
            break;
        case  0x41://ctrl + a
            {
                param = POPU_MENU_ITEM_ABOUT_ID;
            }
        case  0x56:
            {
                param = POPU_MENU_ITEM_HELP_ID;
            }
        default:
            break;
        }

        if (param != 0)
        {
            SendMessageA(g_main_view, WM_COMMAND, param, 0);
        }
    }
}

LRESULT TableDraw (LPARAM lp)
{
    LPNMLVCUSTOMDRAW pListDraw = (LPNMLVCUSTOMDRAW)lp;
    switch(pListDraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW);
    case CDDS_ITEMPREPAINT:
        {
            DWORD col = 0;
            int itm = (int)pListDraw->nmcd.dwItemSpec;

            PacketContent content;
            CFileCache::GetInst()->GetShow(itm, content);
            col = content.m_colour;
            pListDraw->clrTextBk = col;
        }
        return CDRF_NEWFONT;
    default:
        break;
    }
    return CDRF_DODEFAULT;
}

VOID WINAPI OnNotify(WPARAM wp, LPARAM lp)
{
    LPNMHDR msg = (LPNMHDR)lp;
    if (!msg || s_list != msg->hwndFrom)
    {
        return;
    }
    switch(msg->code)
    {
    case NM_RCLICK:
        {
            SendMessageA(g_main_view, MSG_INTERNAL_RBUTTONUP, (WPARAM)s_list, 0);
            //OnListViewRClick(g_main_view);
        }
        break;
    case  NM_CLICK:
        {
            //OnListViewLClick(wp, lp);
        }
        break;
    case  NM_CUSTOMDRAW:
        {
            LPNMLISTVIEW pnm;
            pnm = (LPNMLISTVIEW)lp;
            if (pnm->hdr.hwndFrom == s_list)
            {
                SetWindowLongA(g_main_view, DWL_MSGRESULT, long(TableDraw(lp)));
            }
        }
        break;
    case  LVN_ITEMCHANGED:
        {
            NMLISTVIEW *vm = (NMLISTVIEW *)lp;
            s_last_select = vm->iItem;
            ShowPacketData(vm->iItem);
        }
        break;
    case  LVN_ITEMCHANGING:
        {
        }
        break;
    default:
        break;
    }

    NMLVDISPINFO* plvdi;
    switch (((LPNMHDR) lp)->code)
    {
    case LVN_GETDISPINFO:
        {
            plvdi = (NMLVDISPINFO*)lp;
            OnGetListCtrlDisplsy(plvdi);
        }
        break;
    default:
        break;
    }
}

VOID WINAPI OnDropFiles(WPARAM wp, LPARAM lp)
{
    char file[MAX_PATH] = {0x00};
    DragQueryFile(HDROP(wp), 0, file, MAX_PATH);
    if (0x00 != file[0])
    {
        SendMessageA(g_main_view, MSG_LOAD_PACKETFILE, (WPARAM)file, NULL);
    }
    DragFinish(HDROP(wp));
}

VOID OnAcitveWindow()
{
    if(IsIconic(g_main_view))
    {
        SendMessageA(g_main_view, WM_SYSCOMMAND, SC_RESTORE, 0);
    }
    
    if (!IsZoomed(g_main_view))
    {
        CentreWindow(NULL, g_main_view);
        RECT rect;
        GetWindowRect(g_main_view, &rect);
        if (rect.left < 0 || rect.top < 0)
        {
            SetWindowPos(g_main_view, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    
    if (!g_top_most)
    {
        SetForegroundWindow(g_main_view);
        SetWindowPos(g_main_view, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(g_main_view, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

VOID SendUserSelectMessage(ULONG hbegin, ULONG hend, ULONG sbegin, ULONG send)
{
    USER_SELECT_MSG msg;
    msg.m_hlight_begin = hbegin;
    msg.m_hlight_end = hend;
    msg.m_select_begin = sbegin;
    msg.m_select_end = send;
    if (IsWindow(g_main_view))
    {
        SendMessageA(g_main_view, MSG_UPDATE_SELECT, (WPARAM)&msg, NULL);
    }
}

//hex���û�ѡ��ı�
VOID OnSelectChange(WPARAM wp, LPARAM lp)
{
    if (!wp)
    {
        return;
    }
    USER_SELECT_MSG *select = (USER_SELECT_MSG *)wp;
    int begin = select->m_select_begin;
    int end = select->m_select_end;
    if (begin < 0 || end < 0 || end < begin)
    {
        SetStateMsg(2, "");
        SetStateMsg(3, "");
        return;
    }

    mstring v;
    v.format("λ�ã�%d  ", begin);
    if (end >= begin)
    {
        v += mstring().format("ѡ���ֽڣ�%d", end - begin + 1);
    }
    SetStateMsg(2, v.c_str());
    int count = 0;
    mstring cnt;
    int itm = 0;
    v.clear();

    if (end >= begin && end - begin <= 3)
    {
        if (s_last_select < 0 || s_last_select >= (int)CFileCache::GetInst()->GetShowCount())
        {
            goto leave;
        }

        PacketContent content;
        CFileCache::GetInst()->GetShow(s_last_select, content);
        if (content.m_packet.size() <= (size_t)end)
        {
            goto leave;
        }
        
        DWORD d = 0;
        for (itm = begin ; itm <= end ; itm++)
        {
            d <<= 8;
            d |= (BYTE)content.m_packet.c_str()[itm];
        }
        v.format("ʮ���ƣ�%d  ʮ�����ƣ�%X", d, d);
    }
leave:
    SetStateMsg(3, v.c_str());
}

VOID FramWindow(HWND hwnd)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int w = rc.right - rc.left-1;
    int h = rc.bottom - rc.top-1;
    HDC dc = ::GetWindowDC(hwnd);
    ::SetROP2(dc, R2_XORPEN);
    //HPEN pen = CreatePen(PS_SOLID, 5, RGB(0xff, 0, 0));
    HPEN old = (HPEN)::SelectObject(dc, s_spy_pen);
#define BB 4
    int itm = 0;
    for(itm = 0 ; itm < 4 ; itm++)
    {
        MoveToEx(dc, 0, itm, 0); LineTo(dc, w + 1, itm);
        MoveToEx(dc, 0, h - itm, 0); LineTo(dc, w + 1, h - itm);
        MoveToEx(dc, itm, BB, 0); LineTo(dc, itm, h - BB + 1);
        MoveToEx(dc, w - itm, BB, 0); LineTo(dc, w - itm, h - BB + 1);
    }
    SelectObject(dc, old);
    //DeleteObject((HGDIOBJ)pen);
    ReleaseDC(hwnd, dc);
}

VOID OnMouseMove(HWND hdlg, WPARAM wp, LPARAM lp)
{
    if (s_window_capter)
    {
        POINT pt ={0};
        GetCursorPos(&pt);
        HWND hwnd = WindowFromPoint(pt);
        if (hwnd != s_cur_capter_window)
        {
            if (IsWindow(s_cur_capter_window))
            {
                FramWindow(s_cur_capter_window);
                //RedrawWindow(s_cur_capter_window, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
                //InvalidateRect(s_cur_capter_window, NULL, TRUE);
                //HWND parent = GetParent(s_cur_capter_window);
                //if (IsWindow(parent))
                //{
                //    RedrawWindow(parent, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
                //    InvalidateRect(s_cur_capter_window, NULL, TRUE);
                //} 
            }

            FramWindow(hwnd);
            //RECT rt = {0};
            //GetWindowRect(hwnd, &rt);
            //HDC dc = GetWindowDC(hwnd);
            //HPEN pen = CreatePen(PS_SOLID, 5, RGB(0xff, 0, 0));
            //HPEN old = (HPEN)SelectObject(dc, pen);
            //MoveToEx(dc, 0, 0, NULL);
            //int width = rt.right - rt.left;
            //int height = rt.bottom - rt.top;
            //LineTo(dc, width, 0);
            //LineTo(dc, width, height);
            //LineTo(dc, 0, height);
            //LineTo(dc, 0, 0);
            //SelectObject(dc, old);
            //ReleaseDC(hwnd, dc);
            //DeleteObject(pen);
            s_cur_capter_window = hwnd;
        }
    }
}

static VOID OnLoadPacketFile(LPCSTR szFile)
{
    if (!szFile || !szFile[0])
    {
        return;
    }
    if (g_work_state == em_analysis && !g_analysis_state)
    {
        OnAnalysisDumpFile(szFile);
    }
    else
    {
        char module[MAX_PATH] = {0x00};
        GetModuleFileNameA(NULL, module, MAX_PATH);
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = TRUE;
        mstring cmd;
        cmd.format("%hs /f \"%hs\"", module, szFile);
        BOOL bRet = CreateProcessA(
            NULL,
            (LPSTR)cmd.c_str(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi);
        if(bRet)
        {   
            CloseHandle (pi.hThread);   
            CloseHandle (pi.hProcess);   
        }   
    }
}

VOID OnLButtonUp(HWND hdlg, WPARAM wp, LPARAM lp)
{
    if (s_window_capter)
    {
        ShowWindow(hdlg, SW_SHOW);
        if (IsWindow(s_cur_capter_window))
        {
            RedrawWindow(s_cur_capter_window, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
            HWND parent = GetParent(s_cur_capter_window);
            if(IsWindow(parent))
            {
                RedrawWindow(parent, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            }
            //get process info
            ProcessInfo info;
            DWORD pid = 0;
            GetWindowThreadProcessId(s_cur_capter_window, &pid);
            if (pid)
            {
                if (GetProcessByPid(pid, info.m_path))
                {
                    info.m_pid = pid;
                    mstring ms = info.m_path;
                    ms.repsub("/", "\\");
                    int itm = ms.rfind("\\");
                    info.m_name.assign(ms, itm + 1, ms.size() - itm);
                    ShowProcessStat(g_main_view, info);
                }
            }
        }
        ReleaseCapture();
        s_cur_capter_window = NULL;
        s_window_capter = FALSE;
    }
}

//����������
static VOID OnPacketsOverFlow()
{
    SendMessageW(g_main_view, WM_COMMAND, POPU_MENU_ITEM_SUSPEND_ID, 0);
    MessageBoxW(g_main_view, L"������������������,����ͣ��̽,�뼰ʱ��������", L"�����������", MB_OK | MB_ICONWARNING);
}

//Filter Task
class CFilterTask : public ThreadRunable {
public:
    CFilterTask(const mstring &filterStr) {
        mFilterStr = filterStr;
    }

    virtual ~CFilterTask() {
    }

    virtual void run() {
        list<FilterRules> fltRule;
        do 
        {
            if (!mFilterStr.empty())
            {
                RulesCompile(mFilterStr.c_str(), fltRule);
            }

            g_show_string = mFilterStr;
            if (IsShowRulesDif(fltRule))
            {
                SaveFilterConfig();
                ChangeShowRules(fltRule);

                //Recheck Filter
                int totalCount = CFileCache::GetInst()->GetPacketCount();
                int curPos = 0;
                int showCount = 0;
                CFileCache::GetInst()->ClearShow();
                mstring showStr;
                int maxCount = CFileCache::GetInst()->GetPacketCount();
                for (size_t i = 0 ; i < (size_t)maxCount ; i++)
                {
                    curPos++;
                    PacketContent content;
                    CFileCache::GetInst()->GetPacket(i, content);

                    if (IsPacketPassShow(&content))
                    {
                        showCount++;
                        CFileCache::GetInst()->SetShowPacket(i);
                    }

                    showStr = FormatA("������� %d, �������� %d, �������� %d", totalCount, curPos, showCount);
                    gsProgressDlg->SetProgress(totalCount, curPos, showStr);
                }
            }
        } while (false);

        gsProgressDlg->SetComplete("���з���������");
        gsProgressDlg->Close();
        PostMessageA(g_main_view, MSG_FILTER_COMPLETE, 0, 0);
    }

private:
    mstring mFilterStr;
};

static void _SetMainViewMode(MainViewMode mode) {
    gsViewMode = mode;
    if (em_mode_normal == mode)
    {
        SendMessageA(gsEditFilter, EM_SETREADONLY,  FALSE, 0);
    } else if (em_mode_wait_result == mode)
    {
        SendMessageA(gsEditFilter, EM_SETREADONLY,  TRUE, 0);
    }
}

//���˹�����Ч
static void _OnSetFilter() {
    static mstring sLastRule = "abcdef1234";

    if (em_mode_wait_result == gsViewMode)
    {
        return;
    }

    mstring str = GetWindowStrA(gsEditFilter);
    str.trim();

    if (str == sLastRule)
    {
        return;
    }

    sLastRule = str;
    if (!sLastRule.empty())
    {
        list<FilterRules> flt;
        if (!RulesCompile(sLastRule.c_str(), flt))
        {
            return;
        }
    }
    _SetMainViewMode(em_mode_wait_result);
    gsProgressDlg->CreateDlg(g_main_view);
    gThreadPool->exec(new CFilterTask(sLastRule));
}

static void _OnFilterComplete() {
    UpdatePacketsCount();
    NoticefSnifferViewRefush();
    _SetMainViewMode(em_mode_normal);
}

DWORD CALLBACK ViewProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    int ret = 0;
    switch(msg)
    {
    case  WM_INITDIALOG:
        {
            OnInitDialog(hdlg);
        }
        break;
    case  WM_COMMAND:
        {
            OnCommand(wp, lp);
        }
        break;
    case  WM_TIMER:
        OnTimerEvent(wp, lp);
        break;
        /*
    case  WM_ERASEBKGND:
        return 1;
        */
    case  MSG_UPDATE_ITEM_SPACE:
        {
            OnUpdateMsg(hdlg);
        }
        break;
    case  MSG_UPDATE_DATA:
        {
            SendMessageA(s_list, LVM_SETITEMCOUNT, CFileCache::GetInst()->GetShowCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
        }
        break;
    case  MSG_UPDATE_SELECT:
        {
            OnSelectChange(wp, lp);
        }
        break;
    case  MSG_SEARCH_WINDOW:
        {
            _OnSearchWindow(hdlg, wp, lp);
        }
        break;
    case MSG_LOAD_PACKETFILE:
        {
            OnLoadPacketFile((LPCSTR)wp);
        }
        break;
    case  MSG_PACKETS_OVERFLOW:
        {
            OnPacketsOverFlow();
        }
        break;
    case  MSG_SET_FILTER:
        {
            _OnSetFilter();
        }
        break;
    case MSG_FILTER_COMPLETE:
        {
            _OnFilterComplete();
        }
        break;
    case  WM_MOUSEMOVE:
        {
            OnMouseMove(hdlg, wp, lp);
        }
        break;
    case  WM_LBUTTONUP:
        {
            OnLButtonUp(hdlg, wp, lp);
        }
        break;
    case MSG_INTERNAL_RBUTTONUP:
        {
            OnListViewRClick(g_main_view, wp, lp);
        }
        break;
    case  WM_RBUTTONUP:
        {
            OnRButtonUp(wp, lp);
        }
        break;
    case  WM_KEYDOWN:
        {
            OnKeyDown(wp, lp);
        }
        break;
    case  MSG_STATUS_VERMSG:
        LOCK_STATE;
        SendMessageA(s_status, SB_SETTEXT, 0, (LPARAM)s_state_msg[0].c_str());
        UNLOCK_STATE;
        break;
    case  MSG_STATUS_PACKETS_COUNT:
        LOCK_STATE;
        SendMessageA(s_status, SB_SETTEXT, 1, (LPARAM)s_state_msg[1].c_str());
        UNLOCK_STATE;
        break;
    case  MSG_STATUS_SELECTMSG:
        LOCK_STATE;
        SendMessageA(s_status, SB_SETTEXT, 2, (LPARAM)s_state_msg[2].c_str());
        UNLOCK_STATE;
        break;
    case MSG_STATUS_VALUE:
        LOCK_STATE;
        SendMessageA(s_status, SB_SETTEXT, 3, (LPARAM)s_state_msg[3].c_str());
        UNLOCK_STATE;
    case  WM_NOTIFY:
        {
            ret = 1;
            OnNotify(wp, lp);
        }
        break;
    case  WM_DROPFILES:
        {
            OnDropFiles(wp, lp);
        }
        break;
    case  MSG_ACTIVE_WINDOW:
        {
            OnAcitveWindow();
        }
        break;
    case  WM_CTLCOLOREDIT:
        {
            if (gsEditFilter == (HWND)lp)
            {
                HDC dc = (HDC)wp;
                if (0 == gsEditMode)
                {
                    SetBkColor(dc, gsRightColour);
                    return (INT_PTR)gsBrushRight;
                } else {
                    SetBkColor(dc, gsErrorColour);
                    return (INT_PTR)gsBrushError;
                }
            }
        }
        break;
    case  WM_CLOSE:
        {
            if (g_work_state == em_sniffer)
            {
                OnSnifferExit();
            }
            KillTimer(hdlg, s_timer);
            EndDialog(hdlg, 0);
            DestroyHexView();
            DeleteObject((HGDIOBJ)s_spy_pen);
        }
        break;
    default:
        break;
    }
    return ret;
}

VOID WINAPI ShowSnifferView()
{
    DialogBoxA(g_m, MAKEINTRESOURCEA(IDD_MAIN_VIEW), NULL, (DLGPROC )ViewProc);
    return;
}

VOID WINAPI NoticefSnifferViewRefush()
{
    if (IsWindow(g_main_view))
    {
        PostMessageA(g_main_view, MSG_UPDATE_DATA, 0, 0);
    }

    size_t v;
    v = CFileCache::GetInst()->GetShowCount();
    if (IsWindow(s_list))
    {
        if (v > 0)
        {
            PostMessageA(s_list, LVM_REDRAWITEMS, 0, v - 1);
        }
        else
        {
            PostMessageA(s_list, LVM_REDRAWITEMS, 0, 0);
        }
    }
    InvalidateRect(s_list, NULL, TRUE);

    if (s_last_select > (int)v)
    {
        s_last_select = -1;
    }

    if (IsWindow(s_hex))
    {
        ShowPacketData(s_last_select);
    }
}

VOID WINAPI UpdatePacketsCount()
{
    if (IsWindow(s_status))
    {
        mstring v;
        if(g_work_state == em_sniffer)
        {
            v.format("���������%d  ������ʾ��%d", CFileCache::GetInst()->GetPacketCount(), CFileCache::GetInst()->GetShowCount());
        }
        else if (g_work_state == em_analysis)
        {
            v.format("���������%d  ������ʾ��%d", CFileCache::GetInst()->GetPacketCount(), CFileCache::GetInst()->GetShowCount());
        }
        SetStateMsg(1, v.c_str());
    }
}

VOID WINAPI UpdateHexSelect(DWORD sbegin, DWORD send, DWORD hbegin, DWORD hend)
{
    if (sbegin == -1 || send == -1)
    {
        sbegin = send = 0;
    }

    if (hbegin == -1 || hend == -1)
    {
        hbegin = hend = 0;
    }

    if (IsWindow(s_status))
    {
        mstring v;
        if (send < sbegin)
        {
            send = sbegin;
        }

        if (hend < hbegin)
        {
            hend = hbegin;
        }
        v.format("������Χ��%d-%d  ѡ��Χ��%d-%d", hbegin, hend, sbegin, send);
        SetStateMsg(2, v.c_str());
    }
}

VOID NotifyPacketsOverFlow()
{
    SendMessageW(g_main_view, MSG_PACKETS_OVERFLOW, 0, 0);
}