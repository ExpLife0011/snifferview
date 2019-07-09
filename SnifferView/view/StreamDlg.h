#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <map>
#include <list>
#include "../analysis.h"
#include "../SyntaxHlpr/SyntaxTextView.h"
#include "StreamView.h"
#include "../PacketCache.h"

class CStreamDlg {
public:
    CStreamDlg(int curPos);
    virtual ~CStreamDlg();

    bool Create(HWND parent);
    bool DoModule(HWND parent);

private:
    void InitSyntaxTextView();
    /*Window Message*/
    void OnInitDlg(WPARAM wp, LPARAM lp);
    void OnCommand(WPARAM wp, LPARAM lp);
    void OnClose(WPARAM wp, LPARAM lp);
    INT_PTR OnFindStr(WPARAM wp, LPARAM lp);
    void OnMessage(UINT msg, WPARAM wp, LPARAM lp);
    static INT_PTR CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp);
    static bool PacketEnumHandler(size_t index, const PacketContent *info, void *param);
    static LRESULT CALLBACK EditCtrlProc(HWND, UINT, WPARAM, LPARAM);

private:
    void PrintHex(const mstring &desc, const mstring &content);
    void GetPacketSet();
    void LoadPacketSet(int type);

private:
    std::mstring mLastStr;
    std::list<PacketContent *> mPacketSet;
    PacketAttr mAttr;   //封包属性
    mstring mUnique1;   //正向标识
    mstring mUnique2;   //反向标识

    int mCurSel;
    int mLineMax;
    int mCurPos;
    HWND mParent;
    HWND mhWnd;
    HWND mFind;
    HWND mShowSelect;
    CStreamView mShowView;
    static std::map<HWND, CStreamDlg *> msPtrMap;
    static std::map<HWND, CStreamDlg *> msEditMap;
    PWIN_PROC mOldEditProc;
};

void ShowStreamView(HWND hParent, int curPos);