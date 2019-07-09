#include <WinSock2.h>
#include <Windows.h>
#include <CommCtrl.h>
#include "ProgressDlg.h"
#include "../resource.h"
#include "../common/common.h"

using namespace std;

#define TIMER_PROGRESS_UPDATE       2101
#define MSG_PROGRESS_INIT           (WM_USER + 2011)
#define MSG_PROGRESS_COMPLETE       (WM_USER + 2015)
map<HWND, CProgressDlg *> CProgressDlg::msHandleSet;

CProgressDlg::CProgressDlg() {
    mParent = NULL;

    mTotal = 0;
    mCurPos = 0;
}

CProgressDlg::~CProgressDlg() {
}

bool CProgressDlg::CreateDlg(HWND parent) {
    mParent = parent;
    mTotal = 0, mCurPos = 0;
    mProgressCtrl = NULL;
    mStatusEdit = NULL;
    CreateDialogParamA(NULL, MAKEINTRESOURCEA(IDD_PROGRESS), mParent, DlgProc, (LPARAM)this);
    return true;
}

INT_PTR CProgressDlg::OnInitDlg(WPARAM wp, LPARAM lp) {
    mProgressCtrl = GetDlgItem(mHwnd, IDC_PROGRESS);
    mStatusEdit = GetDlgItem(mHwnd, IDC_PROGRESS_CONTENT);
    SetTimer(mHwnd, TIMER_PROGRESS_UPDATE, 30, NULL);
    CenterWindow(mParent, mHwnd);
    return 0;
}

void CProgressDlg::SetProgress(int total, int curPos, const mstring &content) {
    int lastCount = mTotal;
    //dp(L"test 1:%d, 2:%d", total, curPos);

    mTotal = total;
    mCurPos = curPos;
    mContent = content;

    if (!lastCount && mTotal)
    {
        PostMessageA(mHwnd, MSG_PROGRESS_INIT, 0, 0);
    }
    return;
}

void CProgressDlg::Close() {
    PostMessageA(mHwnd, WM_CLOSE, 0, 0);
}

void CProgressDlg::SetComplete(const mstring &content) {
    KillTimer(mHwnd, TIMER_PROGRESS_UPDATE);

    mContent = content;
    PostMessageA(mHwnd, MSG_PROGRESS_COMPLETE, 0, 0);
}

INT_PTR CProgressDlg::OnCommand(WPARAM wp, LPARAM lp) {
    return 0;
}

INT_PTR CProgressDlg::OnTimer(WPARAM wp, LPARAM lp) {
    if (TIMER_PROGRESS_UPDATE == wp)
    {
        if (0 != mTotal)
        {
            dp(L"dddd2 1:%d, 2:%d", mTotal, mCurPos);
            SendMessageA(mProgressCtrl, PBM_SETRANGE32, 0, mTotal);
            SendMessageA(mProgressCtrl, PBM_SETPOS, mCurPos, 0);
            SetWindowTextA(mStatusEdit, mContent.c_str());
        }
    }
    return 0;
}

INT_PTR CProgressDlg::OnProgressInit() {
    SendMessageA(mProgressCtrl, PBM_SETRANGE32, mTotal, 0);
    SendMessageA(mProgressCtrl, PBM_SETPOS, mCurPos, 0);
    SetWindowTextA(mStatusEdit, mContent.c_str());;
    return 0;
}

INT_PTR CProgressDlg::OnProgressComplete() {
    SendMessageA(mProgressCtrl, PBM_SETRANGE32, 100, 0);
    SendMessageA(mProgressCtrl, PBM_SETPOS, 100, 0);
    SetWindowTextA(mStatusEdit, mContent.c_str());;
    return 0;
}

INT_PTR CProgressDlg::OnClose(WPARAM wp, LPARAM lp) {
    return 0;
}

INT_PTR CProgressDlg::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
    if (WM_INITDIALOG == msg)
    {
        CProgressDlg *ptr = (CProgressDlg *)lp;
        msHandleSet[hdlg] = ptr;
        ptr->mHwnd = hdlg;
    }

    map<HWND, CProgressDlg *>::const_iterator it = msHandleSet.find(hdlg);
    if (it == msHandleSet.end())
    {
        return 0;
    }

    CProgressDlg *ptr = it->second;
    switch (msg) {
        case WM_INITDIALOG:
            {
                ptr->OnInitDlg(wp, lp);
            }
            break;
        case WM_COMMAND:
            {
                ptr->OnCommand(wp, lp);
            }
            break;
        case MSG_PROGRESS_INIT:
            {

                ptr->OnProgressInit();
            }
            break;
        case MSG_PROGRESS_COMPLETE:
            {
                ptr->OnProgressComplete();
            }
            break;
        case WM_TIMER:
            {
                ptr->OnTimer(wp, lp);
            }
            break;
        case WM_CLOSE:
            {
                ptr->OnClose(wp, lp);
                msHandleSet.erase(hdlg);
                EndDialog(hdlg, 0);
            }
            break;
        default:
            break;
    }
    return 0;
}