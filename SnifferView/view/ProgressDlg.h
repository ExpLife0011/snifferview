#pragma once
#include <Windows.h>
#include <map>
#include "../common/mstring.h"

class CProgressDlg {
public:
    CProgressDlg();
    virtual ~CProgressDlg();

    bool CreateDlg(HWND parent);
    void SetProgress(int total, int curPos, const std::mstring &content);
    void SetComplete(const std::mstring &content);
    void Close();
private:
    INT_PTR OnInitDlg(WPARAM wp, LPARAM lp);
    INT_PTR OnCommand(WPARAM wp, LPARAM lp);
    INT_PTR OnTimer(WPARAM wp, LPARAM lp);
    INT_PTR OnProgressInit();
    INT_PTR OnProgressComplete();
    INT_PTR OnClose(WPARAM wp, LPARAM lp);
    static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp);

private:
    HWND mHwnd;
    HWND mParent;
    HWND mProgressCtrl;
    HWND mStatusEdit;
    static std::map<HWND, CProgressDlg *> msHandleSet;
    int mTotal;
    int mCurPos;
    std::mstring mContent;
};