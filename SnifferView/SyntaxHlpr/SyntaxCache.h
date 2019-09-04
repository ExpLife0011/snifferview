#pragma once
#include <Windows.h>
#include <list>
#include <map>
#include <vector>
#include "SyntaxTextView.h"
#include "export.h"
#include "../../ComLib/mstring.h"
#include "../../ComLib/LockBase.h"

class CSyntaxCache : public SyntaxTextView {
public:
    CSyntaxCache();
    virtual ~CSyntaxCache();

    bool InitCache(int interval);
    void PushToCache(const std::mstring &label, const std::mstring &content);

    //clear all cache
    void ClearCache();
private:
    static void CALLBACK TimerCache(HWND hwnd,
        UINT msg,
        UINT_PTR id,
        DWORD time
        );

    struct DataCacheDesc {
        std::mstring mLabel;
        void *mParam;
        size_t mStartPos;
        size_t mLength;

        DataCacheDesc() {
            mStartPos = 0;
            mLength = 0;
            mParam = NULL;
        }
    };
private:
    int mInterval;
    //cache desc
    std::list<DataCacheDesc> mCacheDesc;
    //cache content
    std::mstring mCacheContent;
    //cache locker
    CCriticalSectionLockable mCacheLocker;
    static std::map<HWND, CSyntaxCache *> msTimerCache;
};
