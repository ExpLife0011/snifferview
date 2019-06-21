#pragma once
#include <Windows.h>
#include "../SyntaxHlpr/SyntaxTextView.h"

class CFilterEdit : public SyntaxTextView {
public:
    CFilterEdit();
    virtual ~CFilterEdit();

    bool InitFilterEdit(HWND hParent, int x, int y, int cx, int cy);
private:
};