#include <WinSock2.h>
#include "common/StrUtil.h"
#include "FileCache.h"
#include "PacketCache.h"
#include "global.h"

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
    mstring sql = FormatA("select count(*) from tPacketAttr where pUnique='%hs'", GetInst()->mDbOperator.EncodeStr(msg.m_packet_mark).c_str());
    SqliteResult result = GetInst()->mDbOperator.Select(sql);

    mstring v = result.begin().GetValue("count(*)");
    int count = atoi(result.begin().GetValue("count(*)").c_str());
    if (0 == count)
    {
        GetInst()->InitColour(msg);

        sql = FormatA(
            "insert into tPacketAttr(pUnique, colour, startIndex, endIndex)values('%hs', %d, %d, %d)",
            GetInst()->mDbOperator.EncodeStr(msg.m_packet_mark).c_str(),
            msg.m_colour,
            CFileCache::GetInst()->GetPacketCount(),
            CFileCache::GetInst()->GetPacketCount()
            );
        GetInst()->mDbOperator.Insert(sql);
        GetInst()->mUniqueCount++;
    } else {
        PacketAttr attr;
        GetInst()->GetPacketAttr(msg.m_packet_mark, attr);
        msg.m_colour = attr.mColour;

        sql = FormatA("update tPacketAttr set endIndex=%d where pUnique='%hs'",
            CFileCache::GetInst()->GetPacketCount(),
            GetInst()->mDbOperator.EncodeStr(msg.m_packet_mark).c_str()
            );
        GetInst()->mDbOperator.Update(sql);
    }
    return TRUE;
}

bool CPacketCacheMgr::InitCacheMgr() {
    mDbPath = gCfgPath;
    mDbPath.path_append("CfgCache.db");
    DeleteFileA(mDbPath.c_str());

    mDbOperator.Open(mDbPath);
    mDbOperator.Exec("create table if not exists tPacketAttr (pUnique char(64) PRIMARY KEY, protocol char(16), colour INTEGER, startIndex INTEGER, endIndex INTEGER)");

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
    mDbOperator.Close();
    DeleteFileA(mDbPath.c_str());

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
    mstring sql = FormatA("select colour, startIndex, endIndex from tPacketAttr where pUnique='%hs' limit 1", mDbOperator.EncodeStr(unique).c_str());
    SqliteResult result = mDbOperator.Select(sql);

    if (result.GetSize() != 1)
    {
        return false;
    }

    const SqliteIterator &it = result.begin();
    attr.mColour = atoi(it.GetValue("colour").c_str());
    attr.mStartIndex = atoi(it.GetValue("startIndex").c_str());
    attr.mEndIndex = atoi(it.GetValue("endIndex").c_str());
    return true;
}