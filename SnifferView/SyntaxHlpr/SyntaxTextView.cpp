#include <WinSock2.h>
#include <Windows.h>
#include <fstream>
#include <SyntaxView/include/Scintilla.h>
#include <SyntaxView/include/SciLexer.h>
#include "SyntaxTextView.h"
#include "../../ComLib/common.h"

#pragma comment(lib, "shlwapi.lib")

using namespace std;

typedef int (* SCINTILLA_FUNC) (void*, int, int, int);
typedef void * SCINTILLA_PTR;
map<HWND, SyntaxTextView *> SyntaxTextView::msWinProcCache;
CCriticalSectionLockable *SyntaxTextView::msLocker = NULL;

SyntaxTextView::SyntaxTextView() {
    mLineNum = false;
    mAutoScroll = false;
    mParentProc = NULL;

    if (NULL == msLocker)
    {
        msLocker = new CCriticalSectionLockable();
    }
}

bool SyntaxTextView::ClearHighLight() {
    mHighLight.clear();

    OnViewUpdate();
    return true;
}

void SyntaxTextView::OnViewUpdate() const {
    size_t length = SendMsg(SCI_GETTEXTLENGTH, 0, 0);
    SendMsg(SCI_SETINDICATORCURRENT, NOTE_KEYWORD, 0);
    SendMsg(SCI_INDICATORCLEARRANGE, 0, length);

    if (mHighLight.empty())
    {
        return;
    }

    int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
    int lineCount = SendMsg(SCI_LINESONSCREEN, 0, 0);
    int lastLine = firstLine + lineCount;
    int curLine = firstLine;

    int startPos = SendMsg(SCI_POSITIONFROMLINE, firstLine, 0);
    int lastPos = SendMsg(SCI_GETLINEENDPOSITION, lastLine, 0);

    if (lastPos <= startPos)
    {
        return;
    }

    mstring str = mStrInView.substr(startPos, lastPos - startPos);
    for (map<mstring, DWORD>::const_iterator it = mHighLight.begin() ; it != mHighLight.end() ; it++)
    {
        size_t pos1 = 0;
        size_t pos2 = 0;
        while (mstring::npos != (pos1 = str.find_in_rangei(it->first, pos2))) {
            SendMsg(SCI_INDICATORFILLRANGE, pos1 + startPos, it->first.size());

            pos2 = pos1 + it->first.size();
        }
    }
}

INT_PTR SyntaxTextView::OnNotify(HWND hdlg, WPARAM wp, LPARAM lp) {
    NotifyHeader *header = (NotifyHeader *)lp;
    SCNotification *notify = (SCNotification *)lp;

    switch (header->code) {
        case SCN_UPDATEUI:
            {
                if (notify->updated & SC_UPDATE_SELECTION)
                {
                    size_t pos1 = SendMsg(SCI_GETSELECTIONSTART, 0, 0);
                    size_t pos2 = SendMsg(SCI_GETSELECTIONEND, 0, 0);
                }
                OnViewUpdate();
            }
            break;
        default:
            break;
    }
    return 0;
}

LRESULT SyntaxTextView::WndSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    SyntaxTextView *ptr = NULL;
    {
        CScopedLocker locker(msLocker);
        map<HWND, SyntaxTextView *>::iterator it = msWinProcCache.find(hwnd);
        if (it != msWinProcCache.end())
        {
            ptr = it->second;
        }
    }

    if (NULL == ptr)
    {
        return 0;
    }

    PWIN_PROC pfnOldProc = ptr->mParentProc;
    switch (msg) {
        case WM_NOTIFY:
            ptr->OnNotify(hwnd, wp, lp);
            break;
        case WM_DESTROY:
            {
                CScopedLocker locker(msLocker);
                SetWindowLongPtrA(hwnd, DWLP_DLGPROC, (LONG_PTR)pfnOldProc);
                msWinProcCache.erase(hwnd);
            }
            break;
        default:
            break;
    }
    return CallWindowProc(pfnOldProc, hwnd, msg, wp, lp);;
}

bool SyntaxTextView::CreateView(HWND parent, int x, int y, int cx, int cy) {
    mLineNum = false;
    mLineCount = 0;
    m_parent = parent;
    m_hwnd = CreateWindowExA(
        WS_EX_STATICEDGE,
        "Scintilla",
        "SyntaxTest",
        WS_CHILD | WS_VISIBLE,
        x, y, cx, cy,
        parent,
        NULL,
        NULL,
        NULL
        );

    if (IsWindow(m_hwnd))
    {
        m_pfnSend = (SCINTILLA_FUNC)::SendMessage(m_hwnd, SCI_GETDIRECTFUNCTION, 0, 0);
        m_param = (SCINTILLA_PTR)::SendMessage(m_hwnd, SCI_GETDIRECTPOINTER, 0, 0);

        SendMsg(SCI_SETLEXER, SCLEX_LABEL, 0);
        SendMsg(SCI_SETCODEPAGE, 936, 0);

        //INDIC_ROUNDBOX for HighLight
        SendMsg(SCI_INDICSETSTYLE, NOTE_KEYWORD, INDIC_ROUNDBOX);
        SendMsg(SCI_INDICSETALPHA, NOTE_KEYWORD, 100);
        SendMsg(SCI_INDICSETFORE, NOTE_KEYWORD, RGB(0, 0xff, 0));

        SendMsg(SCI_INDICSETSTYLE, NOTE_SELECT, INDIC_ROUNDBOX);
        SendMsg(SCI_INDICSETALPHA, NOTE_SELECT, 100);
        SendMsg(SCI_INDICSETFORE, NOTE_SELECT, RGB(0, 0, 0xff));

        if (IsWindow(m_parent))
        {
            CScopedLocker locker(msLocker);
            msWinProcCache[m_parent] = this;
            mParentProc = (PWIN_PROC)SetWindowLongPtrA(m_parent, DWLP_DLGPROC, (LONG_PTR)WndSubProc);
        }
    }
    return (TRUE == IsWindow(m_hwnd));
}

bool SyntaxTextView::RegisterParser(const mstring &label, pfnLabelParser parser, void *param) {
    if (!IsWindow(m_hwnd))
    {
        return false;
    }

    LabelParser info;
    info.mLabel = label.c_str();
    info.mParam = param;
    info.mPfnParser = parser;
    SendMsg(MSG_LABEL_REGISTER_PARSER, (WPARAM)&info, 0);
    return true;
}

SyntaxTextView::~SyntaxTextView() {
}

size_t SyntaxTextView::SendMsg(UINT msg, WPARAM wp, LPARAM lp) const {
    return m_pfnSend(m_param, msg, wp, lp);
}

void SyntaxTextView::ClearView() {
    SetText(LABEL_DEFAULT, "");
}

bool SyntaxTextView::AddHighLight(const std::mstring &keyWord, DWORD colour) {
    mHighLight[keyWord] = colour;
    OnViewUpdate();
    return true;
}

void SyntaxTextView::SetStyle(int type, unsigned int textColour, unsigned int backColour) {
    SendMsg(SCI_STYLESETFORE, type, textColour);
    SendMsg(SCI_STYLESETBACK, type, backColour);
}

void SyntaxTextView::ShowMargin(bool bShow) {
    if (bShow)
    {
    } else {
        SendMsg(SCI_SETMARGINWIDTHN, 0, 0);
        SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    }
}

void SyntaxTextView::ShowCaretLine(bool show, unsigned int colour) {
    if (show)
    {
        SendMsg(SCI_SETCARETLINEVISIBLE, TRUE, 0);
        SendMsg(SCI_SETCARETLINEBACK , colour,0);
        SendMsg(SCI_SETCARETLINEVISIBLEALWAYS, 1, 1);
    } else {
        SendMsg(SCI_SETCARETLINEVISIBLE, FALSE, 0);
        SendMsg(SCI_SETCARETLINEVISIBLEALWAYS, 0, 0);
    }
}

void SyntaxTextView::SetDefStyle(unsigned int textColour, unsigned int backColour) {
    SendMsg(SCI_STYLESETFORE, STYLE_DEFAULT, textColour);
    SendMsg(SCI_STYLESETBACK, STYLE_DEFAULT, backColour);
}

void SyntaxTextView::ShowVsScrollBar(bool show) {
    SendMsg(SCI_SETHSCROLLBAR, show, 0);
}

void SyntaxTextView::ShowHsScrollBar(bool show) {
    SendMsg(SCI_SETHSCROLLBAR, show, 0);
}

void SyntaxTextView::SetFont(const std::string &fontName) {
    SendMsg(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)fontName.c_str());
}

void SyntaxTextView::SetCaretColour(unsigned int colour) {
    SendMsg(SCI_SETCARETFORE, (WPARAM)colour, 0);
}

void SyntaxTextView::SetCaretSize(int size) {
    SendMsg(SCI_SETCARETWIDTH, (WPARAM)size, 0);
}

void SyntaxTextView::SetFontWeight(int weight) {
    SendMsg(SCI_STYLESETWEIGHT, STYLE_DEFAULT, weight);
}

int SyntaxTextView::GetFontWeight() {
    return SendMsg(SCI_STYLEGETWEIGHT, STYLE_DEFAULT, 0);
}

unsigned int SyntaxTextView::GetCaretColour() {
    return 0;
}

int SyntaxTextView::SetScrollEndLine() {
    return SendMsg(SCI_SCROLLTOEND, 0, 0);
}

void SyntaxTextView::CheckLineNum() {
    if (!mLineNum)
    {
        return;
    }

    int cur = SendMsg(SCI_GETLINECOUNT, 0, 0);

    int t1 = 1;
    while (cur >= 10) {
        t1++;
        cur /= 10;
    }

    int t2 = 1;
    int cc = mLineCount;
    while (cc >= 10) {
        t2++;
        cc /= 10;
    }

    if (t1 > t2)
    {
        mstring str = "_";
        while (t1 >= 0) {
            t1--;
            str += "0";
        }

        int width = SendMsg(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)str.c_str());
        SendMsg(SCI_SETMARGINWIDTHN, 0, width);
        SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    }
    mLineCount = cur;
}

void SyntaxTextView::ResetLineNum() {
    if (!mLineNum)
    {
        return;
    }

    mLineCount = 0;
    int width = SendMsg(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_0");
    SendMsg(SCI_SETMARGINWIDTHN, 0, width);
    SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    CheckLineNum();
}

void SyntaxTextView::AppendText(const std::mstring &label, const std::mstring &text) {
    LabelNode param;
    param.m_label = label.c_str();
    param.m_content = text.c_str();

    SendMsg(SCI_SETREADONLY, 0, 0);
    size_t length = SendMsg(SCI_GETLENGTH, 0, 0);
    param.m_startPos = length;
    param.m_endPos = length + text.size();

    SendMsg(MSG_LABEL_APPEND_LABEL, 0, (LPARAM)&param);
    SendMsg(SCI_APPENDTEXT, (WPARAM)text.size(), (LPARAM)text.c_str());
    SendMsg(SCI_SETREADONLY, 1, 0);

    mStrInView += text;
    if (mLineNum)
    {
        CheckLineNum();
    }

    if (mAutoScroll)
    {
        SetScrollEndLine();
    }
}

void SyntaxTextView::SetLineNum(bool lineNum) {
    mLineNum = lineNum;

    if (mLineNum)
    {
        int lineCount = SendMsg(SCI_GETLINECOUNT, 0, 0);
        SendMsg(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);

        mstring test = "_";
        while (lineCount-- >= 0) {
            test += "0";
        }

        int w = SendMsg(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)test.c_str());
        SendMsg(SCI_SETMARGINWIDTHN, 0, w);
        SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    } else {
        SendMsg(SCI_SETMARGINWIDTHN, 0, 0);
        SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    }
}

void SyntaxTextView::SetText(const std::mstring &label, const std::mstring &text) {
    LabelNode param;
    param.m_label = label.c_str();
    param.m_content = text.c_str();

    size_t length = SendMsg(SCI_GETLENGTH, 0, 0);
    param.m_startPos = length;
    param.m_endPos = length + text.size();

    SendMsg(SCI_SETREADONLY, 0, 0);
    SendMsg(MSG_LABEL_CLEAR_LABEL, 0, 0);
    SendMsg(MSG_LABEL_APPEND_LABEL, 0, (LPARAM)&param);
    SendMsg(SCI_SETTEXT, 0, (LPARAM)text.c_str());
    SendMsg(SCI_SETREADONLY, 1, 0);
    mStrInView = text;
    ResetLineNum();

    if (mAutoScroll)
    {
        SetScrollEndLine();
    }
}

mstring SyntaxTextView::GetText() const {
    int length = SendMsg(SCI_GETLENGTH, 0, 0);

    MemoryAlloc<char> alloc;
    char *ptr = alloc.GetMemory(length + 1);
    ptr[length] = 0;
    SendMsg(SCI_GETTEXT, length + 1, (LPARAM)ptr);
    return ptr;
}

bool SyntaxTextView::SetSelByRange(size_t startPos, size_t endPos) const {
    size_t line = SendMsg(SCI_LINEFROMPOSITION, startPos, 0);
    SendMsg(SCI_SETSEL, startPos, endPos);

    //SendMsg(SCI_SCROLLRANGE, startPos, endPos);
    //将当前选中内容置于屏幕正中央
    int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
    int lineCount = SendMsg(SCI_LINESONSCREEN, 0, 0);
    int curLine = SendMsg(SCI_LINEFROMPOSITION, startPos, 0);

    int mid = firstLine + (lineCount / 2);
    SendMsg(SCI_LINESCROLL, 0, curLine - mid);
    return true;
}

//从当前选中的位置开始向后查找.如果没有,从当前可见页开始查找
bool SyntaxTextView::JmpNextPos(const mstring &str) {
    int pos1 = SendMsg(SCI_GETSELECTIONSTART, 0, 0);
    int pos2 = SendMsg(SCI_GETSELECTIONEND, 0, 0);

    size_t startPos = 0;
    if (pos2 > pos1)
    {
        startPos = pos2;
    } else {
        int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
        startPos = SendMsg(SCI_POSITIONFROMLINE, firstLine, 0);
    }

    size_t pos3 = mStrInView.find_in_rangei(str, startPos);
    if (mstring::npos == pos3)
    {
        return false;
    }

    return SetSelByRange(pos3, pos3 + str.size());
}

bool SyntaxTextView::JmpLastPos(const mstring &str) {
    /*
    查找算法：
    1.先从当前选择的起始位置开始往前查
    2.如果没有任何选中数据，从当前展示页最后一个字符向前查
    */
    int pos1 = SendMsg(SCI_GETSELECTIONSTART, 0, 0);
    int pos2 = SendMsg(SCI_GETSELECTIONEND, 0, 0);

    size_t startPos = 0;
    if (pos2 > pos1)
    {
        if (pos1 > 1)
        {
            startPos = pos1 - 1;
        } else {
            startPos = pos1;
        }
    } else {
        int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
        int lineCount = SendMsg(SCI_LINESONSCREEN, 0, 0);

        startPos = SendMsg(SCI_GETLINEENDPOSITION, firstLine + lineCount, 0);
    }

    size_t pos3 = mStrInView.rfind_in_rangei(str, startPos);
    if (mstring::npos == pos3)
    {
        return false;
    }

    return SetSelByRange(pos3, pos3 + str.size());
}

bool SyntaxTextView::JmpFirstPos(const mstring &str) {
    return JmpNextPos(str);
}

bool SyntaxTextView::JmpEndPos(const mstring &str) {
    if (str.empty() || mStrInView.empty())
    {
        return false;
    }

    size_t lastPos = 0;
    for (size_t i = mStrInView.size() - 1 ; i != 0 ; i--) {
        if (0 == mStrInView.comparei(str, i))
        {
            lastPos = i;
            break;
        }
    }

    if (lastPos)
    {
        size_t line = SendMsg(SCI_LINEFROMPOSITION, lastPos, 0);

        if (line >= 10)
        {
            SendMsg(SCI_LINESCROLL, 0, line - 10);
        } else {
            SendMsg(SCI_LINESCROLL, 0, line);
        }
        SendMsg(SCI_SETSEL, lastPos, lastPos + str.size());
        return true;
    }
    return false;
}

bool SyntaxTextView::SetAutoScroll(bool flag) {
    mAutoScroll = flag;
    return true;
}

void SyntaxTextView::UpdateView() const {
    SendMsg(SCI_COLOURISE, 0, -1);
}
