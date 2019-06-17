#ifndef SYNTAXSHELL_H_H_
#define SYNTAXSHELL_H_H_
#include <Windows.h>
#include <string>
#include <map>
#include <SyntaxView/include/SciLexer.h>
#include <SyntaxView/include/Scintilla.h>
#include "mstring.h"
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

class SyntaxView {
public:
    SyntaxView();
    virtual ~SyntaxView();

    bool CreateView(HWND parent, int x, int y, int cx, int cy);
    bool RegisterParser(const std::mstring &label, pfnLabelParser parser, void *param);
    size_t SendMsg(UINT msg, WPARAM wp, LPARAM lp) const;
    void AppendText(const std::mstring &label, const std::mstring &text);
    void SetText(const std::mstring &label, const std::mstring &text);
    std::mstring GetText() const;
    void SetLineNum(bool lineNum);
    void ClearView();
    HWND GetWindow() {
        return m_hwnd;
    }

    void CheckLineNum();
    void ResetLineNum();

    void SetStyle(int type, unsigned int textColour, unsigned int backColour);
    void ShowCaretLine(bool show, unsigned int colour);
    void ShowMargin(bool bShow);
    void SetDefStyle(unsigned int textColour, unsigned int backColour);
    void ShowVsScrollBar(bool show);
    void ShowHsScrollBar(bool show);
    void LoadSyntaxCfgFile(const std::mstring path);
    std::string GetFont();
    void SetFont(const std::string &fontName);
    void SetCaretColour(unsigned int colour);
    void SetCaretSize(int size);
    void SetFontWeight(int weight);
    int GetFontWeight();
    unsigned int GetCaretColour();
    int SetScrollEndLine();

private:
    bool mLineNum;
    int mLineCount;

    std::string m_path;
    HWND m_hwnd;
    HWND m_parent;
    SCINTILLA_FUNC m_pfnSend;
    SCINTILLA_PTR m_param;
    std::map<int, std::string> m_SyntaxMap;
};
#endif //SYNTAXSHELL_H_H_