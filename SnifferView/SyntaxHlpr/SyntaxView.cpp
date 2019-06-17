#include <WinSock2.h>
#include <Windows.h>
#include <fstream>
#include <SyntaxView/include/Scintilla.h>
#include <SyntaxView/include/SciLexer.h>
#include "SyntaxView.h"
#include "common.h"

#pragma comment(lib, "shlwapi.lib")

using namespace std;

typedef int (* SCINTILLA_FUNC) (void*, int, int, int);
typedef void * SCINTILLA_PTR;

SyntaxView::SyntaxView() {
    mLineNum = false;
}

bool SyntaxView::CreateView(HWND parent, int x, int y, int cx, int cy) {
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
    }
    return (TRUE == IsWindow(m_hwnd));
}

bool SyntaxView::RegisterParser(const mstring &label, pfnLabelParser parser, void *param) {
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

SyntaxView::~SyntaxView() {
}

size_t SyntaxView::SendMsg(UINT msg, WPARAM wp, LPARAM lp) const {
    return m_pfnSend(m_param, msg, wp, lp);
}

void SyntaxView::ClearView() {
    SetText(LABEL_DEFAULT, "");
}

void SyntaxView::SetStyle(int type, unsigned int textColour, unsigned int backColour) {
    SendMsg(SCI_STYLESETFORE, type, textColour);
    SendMsg(SCI_STYLESETBACK, type, backColour);
}

void SyntaxView::ShowMargin(bool bShow) {
    if (bShow)
    {
    } else {
        SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    }
}

void SyntaxView::ShowCaretLine(bool show, unsigned int colour) {
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

void SyntaxView::SetDefStyle(unsigned int textColour, unsigned int backColour) {
    SendMsg(SCI_STYLESETFORE, STYLE_DEFAULT, textColour);
    SendMsg(SCI_STYLESETBACK, STYLE_DEFAULT, backColour);
}

void SyntaxView::ShowVsScrollBar(bool show) {
    SendMsg(SCI_SETHSCROLLBAR, show, 0);
}

void SyntaxView::ShowHsScrollBar(bool show) {
    SendMsg(SCI_SETHSCROLLBAR, show, 0);
}

std::string SyntaxView::GetFont() {
    char tmp[128];
    tmp[0] = 0x00;
    SendMsg(SCI_STYLEGETFONT, STYLE_DEFAULT, (LPARAM)tmp);
    return tmp;
}

void SyntaxView::SetFont(const std::string &fontName) {
    SendMsg(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)fontName.c_str());
}

void SyntaxView::SetCaretColour(unsigned int colour) {
    SendMsg(SCI_SETCARETFORE, (WPARAM)colour, 0);
}

void SyntaxView::SetCaretSize(int size) {
    SendMsg(SCI_SETCARETWIDTH, (WPARAM)size, 0);
}

void SyntaxView::SetFontWeight(int weight) {
    SendMsg(SCI_STYLESETWEIGHT, STYLE_DEFAULT, weight);
}

int SyntaxView::GetFontWeight() {
    return SendMsg(SCI_STYLEGETWEIGHT, STYLE_DEFAULT, 0);
}

unsigned int SyntaxView::GetCaretColour() {
    return 0;
}

int SyntaxView::SetScrollEndLine() {
    return SendMsg(SCI_SCROLLTOEND, 0, 0);
}

void SyntaxView::CheckLineNum() {
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

void SyntaxView::ResetLineNum() {
    mLineCount = 0;
    int width = SendMsg(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_0");
    SendMsg(SCI_SETMARGINWIDTHN, 0, width);
    SendMsg(SCI_SETMARGINWIDTHN, 1, 0);
    CheckLineNum();
}

void SyntaxView::AppendText(const std::mstring &label, const std::mstring &text) {
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

    if (mLineNum)
    {
        CheckLineNum();
    }
}

void SyntaxView::SetLineNum(bool lineNum) {
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

void SyntaxView::SetText(const std::mstring &label, const std::mstring &text) {
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
    ResetLineNum();
}

mstring SyntaxView::GetText() const {
    int length = SendMsg(SCI_GETLENGTH, 0, 0);

    MemoryAlloc<char> alloc;
    char *ptr = alloc.GetMemory(length + 1);
    ptr[length] = 0;
    SendMsg(SCI_GETTEXT, length + 1, (LPARAM)ptr);
    return ptr;
}