#include <WinSock2.h>
#include <Windows.h>
#include <fstream>
#include <SyntaxView/include/Scintilla.h>
#include <SyntaxView/include/SciLexer.h>
#include <SyntaxView/LexVdebug.h>
#include "SyntaxView.h"
#include "SyntaxDef.h"
#include "../common/common.h"

#pragma comment(lib, "shlwapi.lib")

using namespace std;

typedef int (* SCINTILLA_FUNC) (void*, int, int, int);
typedef void * SCINTILLA_PTR;

SyntaxView::SyntaxView() {
}

bool SyntaxView::CreateView(HWND parent, int x, int y, int cx, int cy) {
    extern HINSTANCE g_hInstance;
    m_parent = parent;
    m_hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
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

        SendMsg(SCI_SETCODEPAGE, 936, 0);
    }
    return (TRUE == IsWindow(m_hwnd));
}

SyntaxView::~SyntaxView() {
}

size_t SyntaxView::SendMsg(UINT msg, WPARAM wp, LPARAM lp) const {
    return m_pfnSend(m_param, msg, wp, lp);
}

void SyntaxView::ClearView() {
    SetText(SCI_LABEL_DEFAULT, "");
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
    } else {
        SendMsg(SCI_SETCARETLINEVISIBLE, FALSE, 0);
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

void SyntaxView::AppendText(const std::mstring &label, const std::mstring &text) const {
    VdebugRuleParam param;
    param.label = label.c_str();
    param.content = text.c_str();

    SendMsg(SCI_SETREADONLY, 0, 0);
    size_t length = SendMsg(SCI_GETLENGTH, 0, 0);
    param.startPos = length;
    param.endPos = length + text.size();

    SendMsg(SCI_APPEND_VDEBUG_TEXT, 0, (LPARAM)&param);
    SendMsg(SCI_APPENDTEXT, (WPARAM)text.size(), (LPARAM)text.c_str());
    SendMsg(SCI_SETREADONLY, 1, 0);
}

void SyntaxView::SetText(const std::mstring &label, const std::mstring &text) const {
    VdebugRuleParam param;
    param.label = label.c_str();
    param.content = text.c_str();

    size_t length = SendMsg(SCI_GETLENGTH, 0, 0);
    param.startPos = length;
    param.endPos = length + text.size();

    SendMsg(SCI_SETREADONLY, 0, 0);
    SendMsg(SCI_SET_VDEBUG_TEXT, 0, (LPARAM)&param);
    SendMsg(SCI_SETTEXT, 0, (LPARAM)text.c_str());
    SendMsg(SCI_SETREADONLY, 1, 0);
}

mstring SyntaxView::GetText() const {
    int length = SendMsg(SCI_GETLENGTH, 0, 0);

    MemoryAlloc<char> alloc;
    char *ptr = alloc.GetMemory(length + 1);
    ptr[length] = 0;
    SendMsg(SCI_GETTEXT, length + 1, (LPARAM)ptr);
    return ptr;
}