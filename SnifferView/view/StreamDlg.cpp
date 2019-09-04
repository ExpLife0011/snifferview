#include "StreamDlg.h"
#include "../resource.h"
#include "../../ComLib/winsize.h"
#include "../../ComLib/common.h"
#include "../../ComLib/StrUtil.h"
#include "../global.h"
#include "../SyntaxHlpr/SyntaxDef.h"
#include "../FileCache/FileCache.h"

using namespace std;

#define MSG_ON_FIND_ENTER       (WM_USER + 1051)

enum StreamShowType {
    em_stream_utf8 = 0,
    em_stream_gbk,
    em_stream_unicode,
    em_stream_hex
};

map<HWND, CStreamDlg *> CStreamDlg::msPtrMap;
map<HWND, CStreamDlg *> CStreamDlg::msEditMap;

CStreamDlg::CStreamDlg(int pos) {
    mCurPos = pos;
    mLineMax = 128;
    mCurSel = -1;
}

CStreamDlg::~CStreamDlg() {
}

bool CStreamDlg::Create(HWND parent) {
    mParent = parent;
    mOldEditProc = NULL;
    CreateDialogParamA(g_m, MAKEINTRESOURCEA(IDD_STREAM), parent, DlgProc, (LPARAM)this);
    return true;
}

bool CStreamDlg::DoModule(HWND parent) {
    mParent = parent;
    DialogBoxParamA(NULL, MAKEINTRESOURCEA(IDD_STREAM), parent, DlgProc, (LPARAM)this);
    return true;
}

void CStreamDlg::InitSyntaxTextView() {
}

LRESULT CStreamDlg::EditCtrlProc(HWND hEdit, UINT msg, WPARAM wp, LPARAM lp) {
    map<HWND, CStreamDlg *>::const_iterator it = msEditMap.find(hEdit);
    if (msEditMap.end() == it)
    {
        return 0;
    }

    CStreamDlg *ptr = it->second;
    if (WM_KEYDOWN == msg)
    {
        if (0x0d == wp)
        {
            SendMessageA(ptr->mhWnd, MSG_ON_FIND_ENTER, 0, 0);
        }
    }
    return CallWindowProc(ptr->mOldEditProc, hEdit, msg, wp, lp);
}

/*Window Message*/
void CStreamDlg::OnInitDlg(WPARAM wp, LPARAM lp) {
    SendMessageA(mhWnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconA(g_m, MAKEINTRESOURCEA(IDI_MAIN)));
    mFind = GetDlgItem(mhWnd, IDC_STREAM_FIND);
    mShowSelect = GetDlgItem(mhWnd, IDC_STREAM_SELECT);

    GetPacketSet();
    SendMessageA(mShowSelect, CB_INSERTSTRING, em_stream_utf8, (LPARAM)"utf-8");
    SendMessageA(mShowSelect, CB_INSERTSTRING, em_stream_gbk, (LPARAM)"gbk");
    SendMessageA(mShowSelect, CB_INSERTSTRING, em_stream_unicode, (LPARAM)"unicode");
    SendMessageA(mShowSelect, CB_INSERTSTRING, em_stream_hex, (LPARAM)"hex");
    SendMessageA(mShowSelect, CB_SETCURSEL, 0, 0);

    RECT clientRect = {0};
    GetClientRect(mhWnd, &clientRect);
    RECT rt1 = {0};
    GetWindowRect(mFind, &rt1);
    MapWindowPoints(NULL, mhWnd, (LPPOINT)&rt1, 2);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHigh = clientRect.bottom - clientRect.top;

    int top = rt1.bottom + 8;
    int bottom = clientRect.bottom - 16;
    mShowView.InitStreamView(mhWnd, rt1.left, rt1.bottom + 8, clientWidth - (rt1.left * 2), bottom - top);
    HWND hShowView = mShowView.GetWindow();
    CTL_PARAMS arry[] = {
        {0, mFind, 0, 0, 1, 0},
        {0, mShowSelect, 1, 0, 0, 0},
        {IDC_STREAM_BTN_NEXT, 0, 1, 0, 0, 0},
        {IDC_STREAM_BTN_LAST, 0, 1, 0, 0, 0},
        {0, hShowView, 0, 0, 1, 1}
    };
    SetCtlsCoord(mhWnd, arry, RTL_NUMBER_OF(arry));
    SetWindowRange(mhWnd, 640, 480);

    int cw = GetSystemMetrics(SM_CXSCREEN);
    int ch = GetSystemMetrics(SM_CYSCREEN);
    int cx = (cw / 4 * 3);
    int cy = (ch / 4 * 3);
    SetWindowPos(mhWnd, HWND_TOP, 0, 0, cx, cy, SWP_NOMOVE);
    CenterWindow(NULL, mhWnd);

    RECT wndRect = {0};
    GetWindowRect(mhWnd, &wndRect);
    SendMessageA(mhWnd, WM_COMMAND, IDC_STREAM_SELECT, 0);

    msEditMap[mFind] = this;
    mOldEditProc = (PWIN_PROC)SetWindowLong(mFind, GWL_WNDPROC, (long)EditCtrlProc);
}

void CStreamDlg::OnCommand(WPARAM wp, LPARAM lp) {
    WORD id = LOWORD(wp);
    if (id == IDC_STREAM_SELECT)
    {
        int curSel = SendMessageA(mShowSelect, CB_GETCURSEL, 0, 0);
        if (mCurSel == curSel)
        {
            return;
        }

        mCurSel = curSel;
        switch (curSel) {
            case em_stream_gbk:
            case em_stream_utf8:
            case em_stream_unicode:
                {
                    mShowView.ClearView();
                    LoadPacketSet(curSel);
                }
                break;
            case em_stream_hex:
                {
                    mShowView.ClearView();
                    LoadPacketSet(curSel);
                }
                break;
        }
    } else if (id == IDC_STREAM_BTN_NEXT)
    {
        mstring str = GetWindowStrA(mFind);
        str.trim();

        if (str.empty())
        {
            return;
        }
        mShowView.JmpNextPos(str);
    } else if (id == IDC_STREAM_BTN_LAST)
    {
        mstring str = GetWindowStrA(mFind);
        str.trim();

        if (str.empty())
        {
            return;
        }
        mShowView.JmpLastPos(str);
    }
}

void CStreamDlg::OnClose(WPARAM wp, LPARAM lp) {
    for (list<PacketContent *>::const_iterator it = mPacketSet.begin() ; it != mPacketSet.end() ; it++)
    {
        delete *it;
    }
    mPacketSet.clear();
    msEditMap.erase(mFind);
}

void CStreamDlg::OnMessage(UINT msg, WPARAM wp, LPARAM lp) {
}

bool CStreamDlg::PacketEnumHandler(size_t index, const PacketContent *info, void *param) {
    CStreamDlg *pThis = (CStreamDlg *)param;

    if (index >= pThis->mAttr.mEndIndex)
    {
        return false;
    }

    if (info->m_packet_mark != pThis->mAttr.mUnique)
    {
        return true;
    }

    PacketContent *ss = new PacketContent;
    *ss = *info;
    pThis->mPacketSet.push_back(ss);

    if (pThis->mUnique2.empty() && info->m_dec_mark != pThis->mUnique1)
    {
        pThis->mUnique2 = info->m_dec_mark;
    }
    return true;
}

INT_PTR CStreamDlg::OnFindStr(WPARAM wp, LPARAM lp) {
    mstring str = GetWindowStrA(mFind);
    str.trim();

    if (mLastStr != str)
    {
        mLastStr = str;
        mShowView.ClearHighLight();
        mShowView.AddHighLight(mLastStr, RGB(0, 0xff, 0));
    }

    if (str.empty())
    {
        return 0;
    }
    mShowView.JmpNextPos(str);
    return 0;
}

INT_PTR CStreamDlg::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    CStreamDlg *ptr = NULL;
    map<HWND, CStreamDlg *>::const_iterator it = msPtrMap.find(hdlg);
    if (it != msPtrMap.end())
    {
        ptr = it->second;
        ptr->OnMessage(msg, wp, lp);
    }

    switch(msg)
    {
    case  WM_INITDIALOG:
        {
            msPtrMap[hdlg] = (CStreamDlg *)lp;
            ptr = (CStreamDlg *)lp;
            ptr->mhWnd = hdlg;
            ptr->OnInitDlg(wp, lp);
        }
        break;
    case WM_COMMAND:
        {
            ptr->OnCommand(wp, lp);
        }
        break;
    case MSG_ON_FIND_ENTER:
        {
            ptr->OnFindStr(wp, lp);
        }
        break;
    case  WM_CLOSE:
        {
            ptr->OnClose(wp, lp);
            EndDialog(hdlg, 0);
            msPtrMap.erase(hdlg);
        }
        break;
    default:
        break;
    }
    return 0;
}

void CStreamDlg::GetPacketSet() {
    if (CFileCache::GetInst()->GetShowCount() <= (size_t)mCurPos)
    {
        return;
    }

    PacketContent content;
    CFileCache::GetInst()->GetShow(mCurPos, content);

    mstring unique = content.m_packet_mark;
    mUnique1 = content.m_dec_mark;

    CPacketCacheMgr::GetInst()->GetPacketAttr(unique, mAttr);
    CFileCache::GetInst()->EnumPacket(PacketEnumHandler, this);
}

void CStreamDlg::PrintHex(const mstring &desc, const mstring &content) {
    mstring showData;
    for (size_t i = 0 ; i < content.size() ; i += 16)
    {
        DWORD dwReadSize = 0;
        if (content.size() < (i + 16))
        {
            dwReadSize = (content.size() - i);
        }
        else
        {
            dwReadSize = 16;
        }

        mstring curLine = content.substr(i, dwReadSize);
        showData += FormatA("%08x  ", i);
        size_t j = 0;
        for (j = 0 ; j < 16 ; j++)
        {
            if (j < dwReadSize)
            {
                showData += FormatA("%02x ", (BYTE)curLine[j]);
            }
            else
            {
                showData += "   ";
            }
        }

        showData += " ";
        showData += GetPrintStr(curLine.c_str(), curLine.size(), false);
        showData += "\n";
    }
    mShowView.PushToCache(desc, showData);
    mShowView.PushToCache(desc, "\n");
}

void CStreamDlg::LoadPacketSet(int type) {
    bool first = true;
    for (list<PPacketContent>::const_iterator it = mPacketSet.begin() ; it != mPacketSet.end() ; it++)
    {
        PacketContent *ptr = *it;
        size_t offset = 0;
        if (ptr->m_tls_type == em_tls_tcp)
        {
            offset = sizeof(IPHeader) + ptr->m_tls_header.m_tcp.get_tcp_header_length();
        }
        else if (ptr->m_tls_type == em_tls_udp)
        {
        }

        mstring desc;
        if (ptr->m_dec_mark == mUnique1)
        {
            desc = LABEL_TCP_PIPE1;
        } else {
            desc = LABEL_TCP_PIPE2;
        }

        mstring showData = ptr->m_packet.substr(offset, ptr->m_packet.size() - offset);
        if (showData.empty())
        {
            continue;
        }

        if (type == em_stream_hex)
        {
            PrintHex(desc, showData);
        } else {
            if (type == em_stream_utf8)
            {
                showData = UtoA(showData);
            } else if (type == em_stream_unicode)
            {
                showData = WtoA(wstring((LPCWSTR)showData.c_str(), showData.size() / 2));
            }

            if (first)
            {
                first = false;
            } else {
                mShowView.PushToCache(desc, "\n");
            }
            mstring show1 = GetPrintStr(showData.c_str(), showData.size());
            mShowView.PushToCache(desc, show1);
        }
    }
}

void ShowStreamView(HWND hParent, int curPos) {
    CStreamDlg *ptr = new CStreamDlg(curPos);
    ptr->Create(hParent);
}