#include "StreamView.h"

CStreamView::CStreamView() {
}

CStreamView::~CStreamView() {
}

void CStreamView::InitStreamView(HWND hParent, int x, int y, int cx, int cy) {
    CreateView(hParent, x, y, cx, cy);

    SetLineNum(false);
    ShowMargin(false);

    InitCache(500);
    SetLineNum(false);
    ShowMargin(false);
    SetCaretColour(RGB(255, 255, 255));

    SetFont("Lucida Console");
    SetCaretSize(1);

    SendMsg(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
    ShowVsScrollBar(true);
    ShowHsScrollBar(true);

    SetStyle(STAT_TCP_PIPE1, RGB(202, 255, 112), RGB(40, 40, 40));
    SetStyle(STAT_TCP_PIPE2, RGB(151, 255, 255), RGB(40, 40, 40));
    SetDefStyle(RGB(255, 0, 0), RGB(40, 40, 40));

    ShowCaretLine(true, RGB(60, 56, 54));
    //pSyntaxView->SendMsg(SCI_SETSELFORE, 1, gs_SelTextColour);
    SendMsg(SCI_SETSELBACK, 1, RGB(255, 255, 255));
    SendMsg(SCI_SETSELALPHA, 70, 0);

    RegisterParser(LABEL_TCP_PIPE1, TcpPipe1Parser, this);
    RegisterParser(LABEL_TCP_PIPE2, TcpPipe2Parser, this);
}

void CStreamView::TcpPipe1Parser(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *sc,
    void *param
    )
{
    sc->SetState(STAT_TCP_PIPE1);
    sc->ForwardBytes(length);
}

void CStreamView::  TcpPipe2Parser(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *sc,
    void *param
    )
{
    sc->SetState(STAT_TCP_PIPE2);
    sc->ForwardBytes(length);
}