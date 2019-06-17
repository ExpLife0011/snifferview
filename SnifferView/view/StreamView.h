#pragma once
#include "../SyntaxHlpr/SyntaxCache.h"

#define LABEL_TCP_PIPE1     "TcpPipe1"
#define LABEL_TCP_PIPE2     "TcpPipe2"

#define STAT_TCP_PIPE1      510
#define STAT_TCP_PIPE2      511

class CStreamView : public CSyntaxCache
{
public:
    CStreamView();
    virtual ~CStreamView();

    void InitStreamView(HWND hParent, int x, int y, int cx, int cy);

private:
    static void __stdcall TcpPipe1Parser(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *s,
        void *param
        );

    static void __stdcall TcpPipe2Parser(
        int initStyle,
        unsigned int startPos,
        const char *ptr,
        int length,
        StyleContextBase *s,
        void *param
        );
};