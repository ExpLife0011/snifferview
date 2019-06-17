#include <WinSock2.h>
#include "SyntaxCache.h"
#include <assert.h>
#include <Shlwapi.h>
#include "LockBase.h"

using namespace std;

#define TIMER_CACHE     (WM_USER + 7001)
map<HWND, CSyntaxCache *> CSyntaxCache::msTimerCache;

CSyntaxCache::CSyntaxCache() {
    mInterval = 0;
    mWorkMode = 0;
    mAutoScroll = false;
    mLineCount = 0;
}

CSyntaxCache::~CSyntaxCache() {
}

bool CSyntaxCache::InitCache(const mstring &label, int interval) {
    mLabel = label;
    mInterval = interval;

    HWND hwnd = SyntaxView::GetWindow();
    assert(IsWindow(hwnd));

    RegisterParser(label, LogParser, this);

    msTimerCache[hwnd] = this;
    SetTimer(hwnd, TIMER_CACHE, interval, TimerCache);

    //INDIC_ROUNDBOX
    SendMsg(SCI_INDICSETSTYLE, NOTE_KEYWORD, INDIC_ROUNDBOX);
    SendMsg(SCI_INDICSETALPHA, NOTE_KEYWORD, 100);
    SendMsg(SCI_INDICSETFORE, NOTE_KEYWORD, RGB(0, 0xff, 0));

    SendMsg(SCI_INDICSETSTYLE, NOTE_SELECT, INDIC_ROUNDBOX);
    SendMsg(SCI_INDICSETALPHA, NOTE_SELECT, 100);
    SendMsg(SCI_INDICSETFORE, NOTE_SELECT, RGB(0, 0, 0xff));
    return true;
}

void CSyntaxCache::TimerCache(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    if (TIMER_CACHE == id)
    {
        map<HWND, CSyntaxCache *>::iterator it = msTimerCache.find(hwnd);

        if (it == msTimerCache.end())
        {
            return;
        }

        CSyntaxCache *ptr = it->second;

        CScopedLocker locker(ptr);
        if (ptr->mCache.empty())
        {
            return;
        }

        ptr->AppendText(ptr->mLabel, ptr->mCache);

        if (ptr->mAutoScroll)
        {
            ptr->SetScrollEndLine();
        }
        ptr->mCache.clear();
    }
}

void CSyntaxCache::SwitchWorkMode(int mode) {
    if (mWorkMode == mode)
    {
        return;
    }

    mWorkMode = mode;

    CScopedLocker locker(this);
    mKeyword.clear();
    mCache.clear();
    mShowData = mContent;
    SetText(mLabel, mShowData);
}

mstring CSyntaxCache::GetFilterStr(const mstring &content, const mstring &key) const {
    size_t pos1 = 0, pos2 = 0;
    size_t pos3 = 0, pos4 = 0;

    if (key.empty())
    {
        return content;
    }

    mstring result;
    while (true) {
        pos1 = content.find_in_rangei(key, pos2);

        if (mstring::npos == pos1)
        {
            break;
        }

        pos3 = content.rfind("\n", pos1);
        if (mstring::npos == pos3)
        {
            pos3 = 0;
        } else {
            pos3++;
        }

        pos4 = content.find("\n", pos1);
        if (mstring::npos == pos4)
        {
            pos4 = content.size() - 1;
        }

        result += content.substr(pos3, pos4 - pos3 + 1);
        pos2 = pos4 + 1;
    }
    return result;
}

void CSyntaxCache::OnFilter() {
    //清理缓存,防止数据重复录入
    mCache.clear();
    if (mKeyword.empty())
    {
        mShowData = mContent;
        SetText(mLabel, mContent);
    } else {
        mShowData = GetFilterStr(mContent, mKeyword);
        SetText(mLabel, mShowData);
    }
    OnViewUpdate();
}

bool CSyntaxCache::SetKeyword(const std::mstring &keyWord) {
    CScopedLocker locker(this);
    if (0 == mWorkMode)
    {
        if (keyWord == mKeyword)
        {
            return true;
        }

        mKeyword = keyWord;
        OnFilter();
        return true;
    } else if (1 == mWorkMode)
    {
        mKeyword = keyWord;
        OnViewUpdate();

        if (JmpNextPos(mKeyword))
        {
            return true;
        }
    }
    return false;
}

mstring CSyntaxCache::GetKeyword() {
    return mKeyword;
}

bool CSyntaxCache::SetAutoScroll(bool flag) {
    mAutoScroll = flag;
    return true;
}

//从当前选中的位置开始向后查找.如果没有,从当前可见页开始查找
bool CSyntaxCache::JmpNextPos(const mstring &str) {
    int pos1 = SendMsg(SCI_GETSELECTIONSTART, 0, 0);
    int pos2 = SendMsg(SCI_GETSELECTIONEND, 0, 0);

    size_t startPos = 0;
    if (pos2 > pos1)
    {
        startPos = pos2;
    } else {
        int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
        startPos = SendMsg(SCI_POSITIONFROMLINE, firstLine, 0);
    }

    size_t pos3 = mShowData.find_in_rangei(str, startPos);
    if (mstring::npos == pos3)
    {
        return false;
    }

    size_t line = SendMsg(SCI_LINEFROMPOSITION, pos3, 0);
    SendMsg(SCI_SETSEL, pos3, pos3 + str.size());

    //将当前选中内容置于屏幕正中央
    int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
    int lineCount = SendMsg(SCI_LINESONSCREEN, 0, 0);
    int curLine = SendMsg(SCI_LINEFROMPOSITION, pos3, 0);

    int mid = firstLine + (lineCount / 2);
    SendMsg(SCI_LINESCROLL, 0, curLine - mid);
    return true;
}

bool CSyntaxCache::JmpFrontPos(const mstring &str) {
    return false;
}

bool CSyntaxCache::JmpFirstPos(const mstring &str) {
    return JmpNextPos(str);
}

bool CSyntaxCache::JmpLastPos(const mstring &str) {
    if (str.empty() || mShowData.empty())
    {
        return false;
    }

    size_t lastPos = 0;
    for (size_t i = mShowData.size() - 1 ; i != 0 ; i--) {
        if (0 == mShowData.comparei(str, i))
        {
            lastPos = i;
            break;
        }
    }

    if (lastPos)
    {
        size_t line = SendMsg(SCI_LINEFROMPOSITION, lastPos, 0);

        if (line >= 10)
        {
            SendMsg(SCI_LINESCROLL, 0, line - 10);
        } else {
            SendMsg(SCI_LINESCROLL, 0, line);
        }
        SendMsg(SCI_SETSEL, lastPos, lastPos + str.size());
        return true;
    }
    return false;
}

void CSyntaxCache::UpdateView() const {
    SendMsg(SCI_COLOURISE, 0, -1);
}

void CSyntaxCache::ClearCache() {
    CScopedLocker locker(this);
    mCache.clear();
    mShowData.clear();
    mContent.clear();
    mLineCount = 0;
}

void CSyntaxCache::GetLineCount(int &all, int &show) const {
    all = mLineCount;
    show = SendMsg(SCI_GETLINECOUNT, 0, 0);

    if (show > 0)
    {
        show--;
    }
}

void CSyntaxCache::OnViewUpdate() const {
    SendMsg(SCI_SETINDICATORCURRENT, NOTE_KEYWORD, 0);
    SendMsg(SCI_INDICATORCLEARRANGE, 0, mShowData.size());

    if (mKeyword.empty())
    {
        return;
    }

    int firstLine = SendMsg(SCI_GETFIRSTVISIBLELINE, 0, 0);
    int lineCount = SendMsg(SCI_LINESONSCREEN, 0, 0);
    int lastLine = firstLine + lineCount;
    int curLine = firstLine;

    int startPos = SendMsg(SCI_POSITIONFROMLINE, firstLine, 0);
    int lastPos = SendMsg(SCI_GETLINEENDPOSITION, lastLine, 0);

    if (lastPos <= startPos)
    {
        return;
    }

    mstring str = mShowData.substr(startPos, lastPos - startPos);

    size_t pos1 = 0;
    size_t pos2 = 0;
    while (mstring::npos != (pos1 = str.find_in_rangei(mKeyword, pos2))) {
        SendMsg(SCI_INDICATORFILLRANGE, pos1 + startPos, mKeyword.size());

        pos2 = pos1 + mKeyword.size();
    }
}

mstring CSyntaxCache::GetViewStr(int startPos, int length) const {
    return mShowData.substr(startPos, length);
}

int CSyntaxCache::GetStrLineCount(const mstring &content) const {
    size_t pos1 = 0, pos2 = 0;

    int count = 0;
    while (true) {
        pos1 = content.find("\n", pos2);

        if (mstring::npos == pos1)
        {
            break;
        }
        count++;
        pos2 = pos1 + 1;
    }

    if (pos2 < content.size())
    {
        count++;
    }
    return count;
}

void CSyntaxCache::PushToCache(const std::mstring &content) {
    CScopedLocker locker(this);
    if (content.empty())
    {
        return;
    }

    int lineCount = GetStrLineCount(content);
    mLineCount += lineCount;
    bool flag = false;
    mstring str = content;
    //避免0x00截断
    for (size_t i = 0 ; i < str.size() ; i++)
    {
        if (str[i] == 0x00) {
            str.replace(i, 1, "null");
        }
    }

    if (0 == mWorkMode)
    {
        if (mKeyword.empty())
        {
            flag = true;
        } else {
            str = GetFilterStr(content, mKeyword);

            if (!str.empty())
            {
                flag = true;
            }
        }
    } else {
        flag = true;
    }

    mContent += content;
    if (content[content.size() - 1] != '\n')
    {
        mContent += "\n";
    }

    if (flag)
    {
        if (!str.empty() && str[str.size() - 1] != '\n')
        {
            str += "\n";
        }

        mCache += str;
        mShowData += str;
    }
}

void CSyntaxCache::LogParser(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *sc,
    void *param
    )
{
    CSyntaxCache *pThis = (CSyntaxCache *)param;

    sc->SetState(STAT_CONTENT);
    sc->ForwardBytes(length);
    return;
}