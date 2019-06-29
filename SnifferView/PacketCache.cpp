#include <WinSock2.h>
#include "common/StrUtil.h"
#include "FileCache/FileCache.h"
#include "PacketCache.h"
#include "global.h"

using namespace std;

CPacketCacheMgr *CPacketCacheMgr::GetInst() {
    static CPacketCacheMgr *sPtr = NULL;

    if (NULL == sPtr)
    {
        sPtr = new CPacketCacheMgr();
    }
    return sPtr;
}

CPacketCacheMgr::CPacketCacheMgr() {
    mInit = false;
    mUniqueCount = 0;
}

CPacketCacheMgr::~CPacketCacheMgr() {
    mDbOperator.Close();
}

void CPacketCacheMgr::InitColour(PacketContent &msg) const {
    CScopedLocker locker(this);

    if (mUniqueCount >= mColourSet.size())
    {
        static DWORD sMagic = 0x12f;
        srand(GetTickCount() + sMagic++);
        BYTE r = rand() % 128 + 128;
        BYTE g = rand() % 128 + 128;
        BYTE b = rand() % 128 + 128;
        msg.m_colour = RGB(r, g, b);
    } else {
        msg.m_colour = mColourSet[mUniqueCount];
    }
}

BOOL CPacketCacheMgr::PacketAttrInit(IN OUT PacketContent &msg) {
    CScopedLocker locker(GetInst());

    map<mstring, PacketAttr>::iterator it;
    if (GetInst()->mAttrSet.end() == (it = GetInst()->mAttrSet.find(msg.m_packet_mark)))
    {
        PacketAttr attr;
        attr.mStartIndex = CFileCache::GetInst()->GetPacketCount();
        attr.mEndIndex = attr.mStartIndex + 1;
        GetInst()->InitColour(msg);
        attr.mColour = msg.m_colour;
        GetInst()->mUniqueCount++;
    } else {
        it->second.mEndIndex = CFileCache::GetInst()->GetPacketCount() + 1;
    }
    return TRUE;
}

bool CPacketCacheMgr::InitCacheMgr() {
    if (mColourSet.empty())
    {
        mColourSet.push_back(RGB(0xff, 0xff, 0xff));
        mColourSet.push_back(RGB(0xff, 0xd0, 0xd0));
        mColourSet.push_back(RGB(0xd0, 0xd0, 0xff));
        mColourSet.push_back(RGB(0xd0, 0xff, 0xd0));
        mColourSet.push_back(RGB(0xcd, 0xcd, 0x00));
        mColourSet.push_back(RGB(0xca, 0xff, 0x70));
        mColourSet.push_back(RGB(0xb0, 0xe2, 0xff));
        mColourSet.push_back(RGB(0x97, 0xff, 0xff));
    }
    mInit = true;
    return mInit;
}

void CPacketCacheMgr::ResetCacheMgr() {
    CScopedLocker locker(this);

    mUniqueCount = 0;
    mColourSet.clear();
    mInit = false;
    InitCacheMgr();
}

bool CPacketCacheMgr::GetPacketAttr(const mstring &unique, PacketAttr &attr) {
    if (!mInit)
    {
        return false;
    }

    CScopedLocker locker(this);
    map<mstring, PacketAttr>::const_iterator it = mAttrSet.find(unique);
    if (mAttrSet.end() == it)
    {
        return false;
    }
    attr = it->second;
    return true;
}