#pragma once
#include "../SyntaxHlpr/SyntaxCache.h"

class CStreamView : public CSyntaxCache
{
public:
    CStreamView();
    virtual ~CStreamView();

    void InitStreamView(HWND hParent, int x, int y, int cx, int cy);

private:
    static LRESULT CALLBACK MyKeyboardProc(int code, WPARAM wp, LPARAM lp);
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