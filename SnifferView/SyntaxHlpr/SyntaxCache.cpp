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
}

CSyntaxCache::~CSyntaxCache() {
}

bool CSyntaxCache::InitCache(int interval) {
    mInterval = interval;

    HWND hwnd = SyntaxTextView::GetWindow();
    assert(IsWindow(hwnd));

    msTimerCache[hwnd] = this;
    SetTimer(hwnd, TIMER_CACHE, interval, TimerCache);
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
        CScopedLocker locker(&ptr->mCacheLocker);
        if (ptr->mCacheContent.empty())
        {
            return;
        }

        for (list<DataCacheDesc>::const_iterator ij = ptr->mCacheDesc.begin() ; ij != ptr->mCacheDesc.end() ; ij++)
        {
            const DataCacheDesc &tmp = *ij;
            ptr->AppendText(tmp.mLabel, ptr->mCacheContent.substr(tmp.mStartPos, tmp.mLength));
        }
        ptr->mCacheDesc.clear();
        ptr->mCacheContent.clear();
    }
}

void CSyntaxCache::ClearCache() {
    CScopedLocker locker(&mCacheLocker);
    mCacheDesc.clear();
    mCacheContent.clear();
}

void CSyntaxCache::PushToCache(const std::mstring &label, const std::mstring &content) {
    if (content.empty())
    {
        return;
    }

    CScopedLocker locker(&mCacheLocker);
    bool newItem = false;
    size_t lastPos = 0;
    if (!mCacheDesc.empty())
    {
        DataCacheDesc &lastDesc = *mCacheDesc.rbegin();

        //Merge content
        if (lastDesc.mLabel == label)
        {
            lastDesc.mLength += content.size();
        } else {
            newItem = true;
            lastPos = lastDesc.mStartPos + lastDesc.mLength;
        }
    } else {
        newItem = true;
    }

    if (newItem)
    {
        DataCacheDesc desc;
        desc.mLabel = label;
        desc.mStartPos = lastPos;
        desc.mLength = content.size();
        mCacheDesc.push_back(desc);
    }
    mCacheContent += content;
}
