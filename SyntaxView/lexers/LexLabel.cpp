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
#include "LexLabel.h"
#include "../common.h"
#include "../SyntaxLabel.h"

using namespace std;

static const char *const pythonWordListDesc[] = {
    "Keywords",
    "Highlighted identifiers",
    0
};

class LexLabelParser {
public:
    static void ColouriseLabelDoc(
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
};

void LexLabelParser::ColouriseLabelDoc(
    unsigned int startPos,
    int length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler
    )
{
    int curLine = styler.GetLine(startPos);
    StyleContext sc(startPos, length, initStyle, styler);

    CSyntaxLabel *mgr = CSyntaxLabel::GetLabelMgr(styler.GetIDocPtr());
    mgr->OnParserStr(startPos, length, sc);
}

void LexLabelParser::FoldPyDoc(
   unsigned int startPos,
   int length,
   int /*initStyle - unused*/,
   WordList *[],
   Accessor &styler
   )
{
}

void __stdcall RegisterSyntaxProc(const char *label, pfnColouriseTextProc pfn) {
}

LexerModule lmLabel(SCLEX_LABEL, LexLabelParser::ColouriseLabelDoc, "labelParser", LexLabelParser::FoldPyDoc, pythonWordListDesc);
