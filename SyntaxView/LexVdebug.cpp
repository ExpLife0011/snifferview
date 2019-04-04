/**
LexVdebug Syntax Rule by lougd 2018-12-6
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexVdebug.h"
#include "common.h"

using namespace std;

static const char *const pythonWordListDesc[] = {
    "Keywords",
    "Highlighted identifiers",
    0
};

struct LexRuleNode {
    string m_content;
    string m_label;
    int m_startPos;
    int m_endPos;
};

class LexVdebugParser {
private:
    LexVdebugParser() {}
    virtual ~LexVdebugParser(){}

public:
    static LexVdebugParser *GetInstance();
    void PushVdebugRule(const VdebugRuleParam *param);
    void ClearVdebugRule();

    void RegisterParser(const string &label, pfnColouriseTextProc pfn);

    static void ColouriseVdebugDoc(
        unsigned int startPos,
        int length,
        int initStyle,
        WordList *keywordlists[],
        Accessor &styler
        );

    static void FoldPyDoc(
        unsigned int startPos,
        int length,
        int /*initStyle - unused*/,
        WordList *[],
        Accessor &styler
        );
private:
    LexRuleNode *GetRuleFromPos(int pos);
    void MoveToNextNode();
    void OnParserStr(const string &lebal, int startPos, const char *content, int size, int initStyle, StyleContext &sc);

private:
    vector<LexRuleNode *> mRuleSet;
    size_t mCurPos;
    map<string, pfnColouriseTextProc> m_parser;
};

LexVdebugParser *LexVdebugParser::GetInstance() {
    static LexVdebugParser *s_ptr = NULL;

    if (s_ptr == NULL)
    {
        s_ptr = new LexVdebugParser();
    }
    return s_ptr;
}

void LexVdebugParser::PushVdebugRule(const VdebugRuleParam *param) {
    if (param->endPos <= param->startPos)
    {
        return;
    }

    LexRuleNode *ptr = new LexRuleNode();
    ptr->m_content = param->content;
    ptr->m_label = param->label;
    ptr->m_startPos = param->startPos;
    ptr->m_endPos = param->endPos;
    mRuleSet.push_back(ptr);

    if (1 == mRuleSet.size())
    {
        mCurPos = 0;
    }
}

void LexVdebugParser::ClearVdebugRule() {
    for (vector<LexRuleNode *>::const_iterator it = mRuleSet.begin() ; it != mRuleSet.end() ; it++)
    {
        delete (*it);
    }
    mRuleSet.clear();
    mCurPos = -1;
}

LexRuleNode *LexVdebugParser::GetRuleFromPos(int pos) {
    //从上次分析的位置继续进行词法分析
    LexRuleNode *ptr = mRuleSet[mCurPos];
    if (pos >= ptr->m_startPos && pos < ptr->m_endPos)
    {
        return ptr;
    }
    return NULL;
}

void LexVdebugParser::MoveToNextNode() {
    mCurPos++;
}

void LexVdebugParser::ColouriseVdebugDoc(
    unsigned int startPos,
    int length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler
    )
{
    int curLine = styler.GetLine(startPos);
    StyleContext sc(startPos, length, initStyle, styler);
    dp(L"startPos:%d, length:%d", startPos, length);
    int startMark = startPos;

    int count2 = length;
    while (true) {
        LexRuleNode *pRuleNode = LexVdebugParser::GetInstance()->GetRuleFromPos(startPos);
        if (!pRuleNode)
        {
            break;
        }

        string sub1;
        string sub2;
        size_t nodeSize = pRuleNode->m_endPos - pRuleNode->m_startPos;
        size_t needSize = pRuleNode->m_endPos - startPos;

        const char *ptr1 = pRuleNode->m_content.c_str() + startPos - pRuleNode->m_startPos;
        if (count2 < (int)needSize)
        {
            sub1 = pRuleNode->m_content.substr(startPos - pRuleNode->m_startPos, count2);
            GetInstance()->OnParserStr(pRuleNode->m_label, startPos, ptr1, count2, sc.GetStat(), sc);
            break;
        } else {
            sub1 = pRuleNode->m_content.substr(startPos - pRuleNode->m_startPos, needSize);
            GetInstance()->OnParserStr(pRuleNode->m_label, startPos, ptr1, needSize, sc.GetStat(), sc);
            count2 -= needSize;
            startPos += needSize;
            LexVdebugParser::GetInstance()->MoveToNextNode();
        }

        if (count2 < 0)
        {
            int eee = 1234;
        }

        if (count2 <= 0)
        {
            break;
        }
    }

    if (startPos - startMark != length)
    {
        int dd = 1234;
    }
    sc.Complete();
}

void LexVdebugParser::FoldPyDoc(
   unsigned int startPos,
   int length,
   int /*initStyle - unused*/,
   WordList *[],
   Accessor &styler
   )
{
    printf("abcdef");
}

void LexVdebugParser::OnParserStr(const string &lebal, int startPos, const char *content, int size, int initStyle, StyleContext &sc) {
    /*
    if (lebal == "TcpPipe1")
    {
        sc.SetState(101);
    } else if (lebal == "TcpPipe2")
    {
        sc.SetState(102);
    }

    sc.ForwardBytes(size);
    return;
    */
    map<string, pfnColouriseTextProc>::const_iterator it = m_parser.find(lebal);
    if (it != m_parser.end())
    {
        it->second(initStyle, startPos, content, size, &sc);
    }
}

void LexVdebugParser::RegisterParser(const string &label, pfnColouriseTextProc pfn) {
    m_parser[label] = pfn;
}

void ClearVdebugRule() {
    LexVdebugParser::GetInstance()->ClearVdebugRule();
}

void PushVdebugRule(VdebugRuleParam *ptr) {
    LexVdebugParser::GetInstance()->PushVdebugRule(ptr);
}

void __stdcall RegisterSyntaxProc(const char *label, pfnColouriseTextProc pfn) {
    LexVdebugParser::GetInstance()->RegisterParser(label, pfn);
}

LexerModule lmVdebug(SCLEX_VDEBUG, LexVdebugParser::ColouriseVdebugDoc, "vdebug", LexVdebugParser::FoldPyDoc, pythonWordListDesc);