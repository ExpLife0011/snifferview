#pragma once
#include <string>
#include <map>
#include <vector>
#include "export.h"
#include "Scintilla.h"

using namespace std;

typedef const void* HLabel;

struct LabelCache {
    string mLabel;
    string mContent;

    size_t mStartPos;
    size_t mEndPos;
};

class CSyntaxLabel {
    struct ParserInfo {
        string mLabel;
        void *mParam;
        pfnColouriseTextProc mParser;
    };

public:
    static CSyntaxLabel *GetLabelMgr(HLabel indee);
    static void RegisterLabel(HLabel index);
    static void UnRegisterLabel(HLabel  index);

    void SetStrKeyword(const string &str);
    void RegisterParser(const LabelParser *parser);
    void PushLabel(const void *ptr);
    void ClearLabel();
    const LabelCache *GetLabelNode(int pos) const;
    void MoveNextPos();
    void OnParserStr(size_t startPos, size_t size, StyleContextBase &sc);

private:
    CSyntaxLabel();
    virtual ~CSyntaxLabel();

private:
    size_t mCurPos;
    vector<LabelCache *> mNodeSet;
    map<string, ParserInfo> mParserSet;
    string mStrKeyword;

private:
    static map<HLabel, CSyntaxLabel *> msMgrSet;
};