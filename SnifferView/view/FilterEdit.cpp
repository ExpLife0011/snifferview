#include "FilterEdit.h"

CFilterEdit::CFilterEdit() {
}

CFilterEdit::~CFilterEdit() {
}

bool CFilterEdit::InitFilterEdit(HWND hParent, int x, int y, int cx, int cy) {
    CreateView(hParent, x, y, cx, cy);
    return true;
}