#pragma once
#include <Windows.h>
#include <vector>
#include "analysis.h"
#include "mstring.h"
#include "LockBase.h"

typedef bool (* pfnPacketEnumHandler)(size_t index, const PacketContent *info, void *param);

/*
接口设计:
单个封包内容获取接口由于通常非密集获取，为保证线程安全通过copy对象返回。
枚举封包内容为保证效率，通过回调直接返回指针，避免频繁copy。
*/
class CFileCache {
    struct PacketPosInFile {
        bool mInCache;

        PPacketContent mPtr;
        size_t mStartPos;
        size_t mEndPos;

        PacketPosInFile() {
            mStartPos = 0, mEndPos = 0;
            mInCache = false, mPtr = NULL;
        }
    };

public:
    static CFileCache *GetInst();
    bool InitFileCache();

    size_t GetPacketCount() const;
    //由索引获取封包结构
    bool GetPacket(size_t index, PacketContent &content) const;
    //枚举封包结构接口
    bool EnumPacket(pfnPacketEnumHandler pfnHandler, void *param = NULL);

    size_t GetShowCount() const;
     //由索引获取封包结构
    bool GetShow(size_t index, PacketContent &content) const;

    //清除展示缓存
    void ClearShow();
    void SetShowPacket(size_t index);
    void ClearCache();
    void PushPacket(const PacketContent &packet, bool show);
private:
    CFileCache();
    virtual ~CFileCache();

    void PushToFile(const PacketContent &packet, PacketPosInFile &pos) ;
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
    PacketContent mTempContent;

    CCriticalSectionLockable mCacheLocker;  //Cache Locker
    DWORD mMaxCache;
};