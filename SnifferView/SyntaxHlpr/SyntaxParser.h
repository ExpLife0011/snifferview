#ifndef SYNTAXPARSER_H_H_
#define SYNTAXPARSER_H_H_
#include <Windows.h>
#include <string>
#include <list>
#include "../../SyntaxView/export.h"

typedef void (__stdcall *pfnRegisterSyntaxProc)(const char *label, pfnColouriseTextProc pfn);

struct SyntaxParserMsg {
    int m_stat;
    int m_startPos;
    int m_endPos;
    int m_length;
    std::string m_content;
};

class SyntaxParser {
private:
    SyntaxParser();
    virtual ~SyntaxParser();

public:
    static SyntaxParser *GetInstance();
    bool InitParser();

private:
    static bool IsValidChar(char c);
    static std::list<SyntaxParserMsg> CallStackParserForLine(unsigned int startPos, const std::string &content);
    bool RegisterSyntaxProc(const std::string &label, pfnColouriseTextProc pfn);
    static std::list<SyntaxParserMsg> SplitByLine(int startPos, int length, const std::string &content);
    static void __stdcall CallStackParser(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *sc
        );
    static void __stdcall DefaultParser(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *sc
        );
    static void __stdcall TcpPipe1(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *sc
        );
    static void __stdcall TcpPipe2(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *sc
        );

private:
    pfnRegisterSyntaxProc m_pfnRegister;
};
#endif //SYNTAXPARSER_H_H_