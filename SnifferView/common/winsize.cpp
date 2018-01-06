#include <map>
#include <list>
#include "winsize.h"

using namespace std;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);

struct CTL_MSG
{
    HWND m_hwnd;
    ULONG m_width;
    ULONG m_high;
    ULONG m_x;
    ULONG m_y;
    FLOAT m_cf_x;
    FLOAT m_cf_y;
    FLOAT m_cf_cx;
    FLOAT m_cf_cy;
};

#define MARK_DYN_WIN  (0x01 << 1)
#define MARK_SIZE_WIN (0x01 << 2)

struct WND_MSG
{
    DWORD m_mark;
    LONG m_w;
    LONG m_h;
    LONG m_max_width;
    LONG m_min_width;
    LONG m_max_high;
    LONG m_min_high;
    PWIN_PROC m_proc;
    list<CTL_MSG> m_ctls;
};

static map<HWND, WND_MSG> s_wnds;
static HANDLE s_wnds_lock = CreateMutex(0, FALSE, NULL);
#define LOCK_WINS   WaitForSingleObject(s_wnds_lock, INFINITE)
#define UNLOCK_WINS ReleaseMutex(s_wnds_lock)

static VOID _OnSize(HWND hwnd, WPARAM wp, LPARAM lp)
{
    LOCK_WINS;
    if (s_wnds.find(hwnd) != s_wnds.end() && (s_wnds[hwnd].m_mark & MARK_SIZE_WIN))
    {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int hight = rect.bottom - rect.top;
        int w = width;
        int h = hight;

        if (width > s_wnds[hwnd].m_max_width)
        {
            w = s_wnds[hwnd].m_max_width;
        }

        if (width < s_wnds[hwnd].m_min_width)
        {
            w = s_wnds[hwnd].m_min_width;
        }

        if (hight > s_wnds[hwnd].m_max_high)
        {
            h = s_wnds[hwnd].m_max_high;
        }

        if (hight < s_wnds[hwnd].m_min_high)
        {
            h = s_wnds[hwnd].m_min_high;
        }

        if (width != w || hight != h)
        {
            SetWindowPos(hwnd, 0, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        }
    }

    if (s_wnds.find(hwnd) != s_wnds.end() && (s_wnds[hwnd].m_mark & MARK_DYN_WIN))
    {
        if (IsIconic(hwnd))
        {
            goto leave;
        }
        RECT rect;
        GetWindowRect(hwnd, &rect);
        int w = (rect.right - rect.left) - s_wnds[hwnd].m_w;
        int h = (rect.bottom - rect.top) - s_wnds[hwnd].m_h;

        list<CTL_MSG>::iterator itm;
        for (itm = s_wnds[hwnd].m_ctls.begin() ; itm != s_wnds[hwnd].m_ctls.end() ; itm++)
        {
            CTL_MSG info = *itm;
            MoveWindow(
                info.m_hwnd, info.m_x + (int)(info.m_cf_x * w),
                info.m_y + (int)(info.m_cf_y * h),
                info.m_width + (int)(info.m_cf_cx * w),
                info.m_high + (int)(info.m_cf_cy * h),
                TRUE
                );
            //ÖØ»æ´°Ìå
            InvalidateRect(info.m_hwnd, NULL, TRUE);
        }
    }
leave:
    UNLOCK_WINS;
}

static VOID _OnSizing(HWND hwnd, WPARAM wp, LPARAM lp)
{
    LOCK_WINS;
    if (s_wnds.find(hwnd) != s_wnds.end() && (s_wnds[hwnd].m_mark & MARK_SIZE_WIN))
    {
        if (IsIconic(hwnd))
        {
            goto leave;
        }

        RECT *rect = (RECT *)lp;
        int w = rect->right - rect->left;
        int h = rect->bottom - rect->top;
        bool top = true;
        bool left = true;

        if (wp == WMSZ_BOTTOM || wp == WMSZ_BOTTOMLEFT || wp == WMSZ_BOTTOMRIGHT)
        {
            top = false;
        }

        if (wp == WMSZ_RIGHT || wp == WMSZ_BOTTOMRIGHT || wp == WMSZ_TOPRIGHT)
        {
            left = false;
        }

        if (w > s_wnds[hwnd].m_max_width)
        {
            if (left)
            {
                rect->left += (w -  s_wnds[hwnd].m_max_width);
            }
            else
            {
                rect->right -= (w - s_wnds[hwnd].m_max_width);
            }
        }

        if (w < s_wnds[hwnd].m_min_width)
        {
            if (left)
            {
                rect->left -= (s_wnds[hwnd].m_min_width - w);
            }
            else
            {
                rect->right += (s_wnds[hwnd].m_min_width - w);
            }
        }

        if (h > s_wnds[hwnd].m_max_high)
        {
            if (top)
            {
                rect->top += (h - s_wnds[hwnd].m_max_high);
            }
            else
            {
                rect->bottom -= (h - s_wnds[hwnd].m_max_high);
            }
        }

        if (h < s_wnds[hwnd].m_min_high)
        {
            if (top)
            {
                rect->top -= (s_wnds[hwnd].m_min_high - h);
            }
            else
            {
                rect->bottom += (s_wnds[hwnd].m_min_high - h);
            }
        }
    }
leave:
    UNLOCK_WINS;
}

static VOID _OnDestroy(HWND hwd, WPARAM wp, LPARAM lp)
{    
    LOCK_WINS;
    PWIN_PROC proc = NULL;
    if (s_wnds.find(hwd) != s_wnds.end())
    {
        proc = s_wnds[hwd].m_proc;
        s_wnds.erase(hwd);
    }
    UNLOCK_WINS;;

    if (proc)
    {
        CallWindowProc(proc, hwd, WM_DESTROY, wp, lp);
    }    
}

static LRESULT CALLBACK _WndSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
    case WM_SIZE:
        _OnSize(hwnd, wp, lp);
        break;
    case  WM_SIZING:
        _OnSizing(hwnd, wp, lp);
        break;
    case WM_DESTROY:
        _OnDestroy(hwnd, wp, lp);
        return 0;
    default:
        break;
    }

    LOCK_WINS;
    if (s_wnds.find(hwnd) != s_wnds.end())
    {
        UNLOCK_WINS;
        return CallWindowProc(s_wnds[hwnd].m_proc, hwnd, msg, wp, lp);
    }
    UNLOCK_WINS;
    return 0;
}

static BOOL _CheckWnd(HWND hdlg)
{
    if (!IsWindow(hdlg))
    {
        if (s_wnds.find(hdlg) != s_wnds.end())
        {
            s_wnds.erase(hdlg);
        }
        return FALSE;
    }
    return TRUE;
}

static BOOL _AttachWnd(HWND hdlg)
{
    WND_MSG info;
    RECT rect;
    GetWindowRect(hdlg, &rect);
    info.m_w = rect.right - rect.left;
    info.m_h = rect.bottom - rect.top;
    if (s_wnds.find(hdlg) == s_wnds.end())
    {
        info.m_proc = (PWIN_PROC)SetWindowLongPtrA(hdlg, DWLP_DLGPROC, (LONG_PTR)_WndSubProc);
        info.m_mark = MARK_DYN_WIN;
        if (!info.m_proc)
        {
            return FALSE;
        }
        s_wnds[hdlg] = info;
    }
    else
    {
        s_wnds[hdlg].m_w = info.m_w;
        s_wnds[hdlg].m_h = info.m_h;
        s_wnds[hdlg].m_mark |= MARK_DYN_WIN;
    }
    return TRUE;
}

static VOID _PushCtrlInlist(const CTL_MSG &ctl, list<CTL_MSG> &lst)
{
    list<CTL_MSG>::iterator itm;
    itm = lst.begin();
    while (itm != lst.end())
    {
        if (ctl.m_hwnd == itm->m_hwnd)
        {
            itm = lst.erase(itm);
        }
        else
        {
            itm++;
        }
    }
    lst.push_back(ctl);
}

static BOOL _AttachCtls(HWND hdlg, PCTL_PARAMS arry, size_t count)
{
    BOOL ret = FALSE;
    int it;
    for (it = 0 ; it < (int)count ; it++)
    {
        CTL_MSG info;
        PCTL_PARAMS st = (arry + it);
        info.m_cf_x = st->m_x;
        info.m_cf_y = st->m_y;
        info.m_cf_cx = st->m_cx;
        info.m_cf_cy = st->m_cy;
        if (st->m_id)
        {
            info.m_hwnd = GetDlgItem(hdlg, st->m_id);
        }
        else
        {
            info.m_hwnd = st->m_hwnd;
        }

        if (!IsWindow(info.m_hwnd))
        {
            continue;
        }

        RECT rect;
        GetWindowRect(info.m_hwnd, &rect);
        POINT pt;
        pt.x = rect.left;
        pt.y = rect.top;
        ScreenToClient(hdlg, &pt);
        info.m_x = pt.x;
        info.m_y = pt.y;
        info.m_width = rect.right - rect.left;
        info.m_high = rect.bottom - rect.top;
        _PushCtrlInlist(info, s_wnds[hdlg].m_ctls);
        ret = TRUE;
    }
    return ret;
}

BOOL WINAPI SetCtlsCoord(HWND hdlg, PCTL_PARAMS arry, DWORD count)
{
    BOOL ret = FALSE;
    LOCK_WINS;
    if (!_CheckWnd(hdlg))
    {
        goto leave;
    }

    if (!_AttachWnd(hdlg))
    {
        goto leave;
    }

    ret = _AttachCtls(hdlg, arry, count);
leave:
    UNLOCK_WINS;
    return ret;
}

static BOOL _AttachRange(HWND hwnd, DWORD min_wide, DWORD min_hight, DWORD max_wide, DWORD max_hight)
{
    if (s_wnds.find(hwnd) != s_wnds.end())
    {
        s_wnds[hwnd].m_mark |= MARK_SIZE_WIN;
        s_wnds[hwnd].m_max_high = max_hight;
        s_wnds[hwnd].m_max_width = max_wide;
        s_wnds[hwnd].m_min_high = min_hight;
        s_wnds[hwnd].m_min_width = min_wide;
    }
    else
    {
        PWIN_PROC proc = (PWIN_PROC)SetWindowLongPtrA(hwnd, DWLP_DLGPROC, (LONG_PTR)_WndSubProc);(hwnd, DWLP_DLGPROC, (LONG_PTR)_WndSubProc);
        if (!proc)
        {
            return FALSE;
        }
        s_wnds[hwnd].m_mark = MARK_SIZE_WIN;
        s_wnds[hwnd].m_max_high = max_hight;
        s_wnds[hwnd].m_max_width = max_wide;
        s_wnds[hwnd].m_min_high = min_hight;
        s_wnds[hwnd].m_min_width = min_wide;
        s_wnds[hwnd].m_proc = proc;
    }
    return TRUE;
}

BOOL WINAPI SetWindowRange(HWND hwnd, DWORD min_wide, DWORD min_hight, DWORD max_wide, DWORD max_hight)
{
    if (!IsWindow(hwnd))
    {
        return FALSE;
    }

    if (max_wide == 0)
    {
        max_wide = 65536;
    }

    if(max_hight == 0)
    {
        max_hight = 65536;
    }

    LOCK_WINS;
    BOOL ret = _AttachRange(hwnd, min_wide, min_hight, max_wide, max_hight);
    UNLOCK_WINS;
    return ret;
}