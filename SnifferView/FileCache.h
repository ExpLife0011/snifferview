#pragma once
#include <Windows.h>
#include <vector>
#include "analysis.h"
#include "mstring.h"
#include "LockBase.h"

class CFileCache {
    struct PacketPosInFile {
        size_t mStartPos;
        size_t mEndPos;

        PacketPosInFile() {
            mStartPos = 0, mEndPos = 0;
        }
    };

public:
    static CFileCache *GetInst();
    bool InitFileCache();

    size_t GetPacketCount() const;
    bool GetPacket(size_t index, PacketContent &packet) const;

    size_t GetShowCount() const;
    bool GetShow(size_t index, PacketContent &packet) const;

    //Çå³ýÕ¹Ê¾»º´æ
    void ClearShow();
    void SetShowPacket(size_t index);
    void ClearCache();
    void PushPacket(const PacketContent &packet, bool show);
private:
    void Encode(std::mstring &str) const;
    void Decode(std::mstring &str) const;

    bool GetPacketFromStr(const std::mstring &str, PacketContent &packet) const;
    std::mstring GetContent(const PacketPosInFile &pos) const;

private:
    std::mstring mCacheFile;
    //Write Handle, Write File with Locker and Read Without.
    HANDLE mWriteHandle;
    std::vector<PacketPosInFile> mShowSet;
    std::vector<PacketPosInFile> mPacketSet;

    CCriticalSectionLockable mWriteLocker;  //Write Locker
    CCriticalSectionLockable mCacheLocker;  //Cache Locker
};