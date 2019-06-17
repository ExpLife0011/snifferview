#ifndef SYNTAXSHELL_H_H_
#define SYNTAXSHELL_H_H_
#include <Windows.h>
#include <string>
#include <map>
#include <SyntaxView/include/SciLexer.h>
#include <SyntaxView/include/Scintilla.h>
#include "mstring.h"
#include "LockBase.h"
#include "SyntaxDef.h"
#include "export.h"

typedef int (* SCINTILLA_FUNC) (void*, int, int, int);
typedef void * SCINTILLA_PTR;
typedef void (__stdcall *pfnLabelParser)(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *s,
    void *param
    );

#define SCLEX_LABEL 161
//LabelParser消息
//为SyntaxView注册解析器
//wparem : LabelParser
//lparam : no used
/*
struct LabelParser {
    const char *mLabel;
    void *mParam;
    void *mPfnParser;
};
*/
#define MSG_LABEL_REGISTER_PARSER     5051
//设置文本标签
#define MSG_LABEL_CLEAR_LABEL         5060
//追加文本标签
#define MSG_LABEL_APPEND_LABEL        5061
//设置高亮字符串
//wparam : const char *
//lparam : no used
#define MSG_SET_KEYWORK_STR           5071

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);

class SyntaxView : public CCriticalSectionLockable {
public:
    SyntaxView();
    virtual ~SyntaxView();

    //SyntaxView Create
    bool CreateView(HWND parent, int x, int y, int cx, int cy);
    //Parser Register
    bool RegisterParser(const std::mstring &label, pfnLabelParser parser, void *param);
    //Send Window Message
    size_t SendMsg(UINT msg, WPARAM wp, LPARAM lp) const;
    //AppendText To View
    void AppendText(const std::mstring &label, const std::mstring &text);
    //Set View Text
    void SetText(const std::mstring &label, const std::mstring &text);
    //Get Current View Text
    std::mstring GetText() const;
    //Set Line Num
    void SetLineNum(bool lineNum);
    void ClearView();
    HWND GetWindow() {
        return m_hwnd;
    }

    //update view
    void UpdateView() const;

    //Auto to EndLine
    bool SetAutoScroll(bool flag);
    //Sel Jump To Next Str
    bool JmpNextPos(const std::mstring &str);
    bool JmpFrontPos(const std::mstring &str);
    bool JmpFirstPos(const std::mstring &str);
    bool JmpLastPos(const std::mstring &str);

    //Set Keyword For HighLight
    bool AddHighLight(const std::mstring &keyWord, DWORD colour);
    bool ClearHighLight();

    void SetStyle(int type, unsigned int textColour, unsigned int backColour);
    void ShowCaretLine(bool show, unsigned int colour);
    void ShowMargin(bool bShow);
    void SetDefStyle(unsigned int textColour, unsigned int backColour);
    void ShowVsScrollBar(bool show);
    void ShowHsScrollBar(bool show);

    void SetFont(const std::string &fontName);
    void SetCaretColour(unsigned int colour);
    void SetCaretSize(int size);
    void SetFontWeight(int weight);
    int GetFontWeight();
    unsigned int GetCaretColour();
    int SetScrollEndLine();

private:
    void OnViewUpdate() const;
    INT_PTR OnNotify(HWND hdlg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK WndSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void CheckLineNum();
    void ResetLineNum();

private:
    PWIN_PROC mParentProc;
    static CCriticalSectionLockable *msLocker;
    static std::map<HWND, SyntaxView *> msWinProcCache;

    bool mLineNum;
    int mLineCount;
    bool mAutoScroll;

    HWND m_hwnd;
    HWND m_parent;
    SCINTILLA_FUNC m_pfnSend;
    SCINTILLA_PTR m_param;
    std::map<std::mstring, DWORD> mHighLight;
    std::mstring mStrInView;
};
#endif //SYNTAXSHELL_H_H_