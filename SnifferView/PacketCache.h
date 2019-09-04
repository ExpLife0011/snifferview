/*
2019/06/20
Packet Cache, example colour, index range,...
*/
#pragma once
#include <Windows.h>
#include <vector>
#include "../ComLib/mstring.h"
#include "../ComLib/LockBase.h"
#include "../ComLib/SqliteOperator.h"
#include "analysis.h"

struct PacketAttr {
    std::mstring mUnique;
    DWORD mColour;
    DWORD mStartIndex;
    DWORD mEndIndex;

    PacketAttr() {
        mColour = 0;
        mStartIndex = 0;
        mEndIndex = 0;
    }
};

class CPacketCacheMgr : public CCriticalSectionLockable {
public:
    static CPacketCacheMgr *GetInst();
    static BOOL WINAPI PacketAttrInit(IN OUT PacketContent &msg);

    bool InitCacheMgr();
    void ResetCacheMgr();
    bool GetPacketAttr(const std::mstring &unique, PacketAttr &attr);
private:
    CPacketCacheMgr();
    virtual ~CPacketCacheMgr();
    void InitColour(PacketContent &msg) const;

private:
    bool mInit;
    SqliteOperator mDbOperator;
    std::mstring mDbPath;
    size_t mUniqueCount;
    std::vector<DWORD> mColourSet;
    std::map<std::mstring, PacketAttr> mAttrSet;
};