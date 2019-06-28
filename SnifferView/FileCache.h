#pragma once
#include <Windows.h>
#include <vector>
#include "analysis.h"
#include "mstring.h"
#include "LockBase.h"

typedef bool (* pfnPacketEnumHandler)(size_t index, const PacketContent *info, void *param);

/*
�ӿ����:
����������ݻ�ȡ�ӿ�����ͨ�����ܼ���ȡ��Ϊ��֤�̰߳�ȫͨ��copy���󷵻ء�
ö�ٷ������Ϊ��֤Ч�ʣ�ͨ���ص�ֱ�ӷ���ָ�룬����Ƶ��copy��
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
    //��������ȡ����ṹ
    bool GetPacket(size_t index, PacketContent &content) const;
    //ö�ٷ���ṹ�ӿ�
    bool EnumPacket(pfnPacketEnumHandler pfnHandler, void *param = NULL);

    size_t GetShowCount() const;
     //��������ȡ����ṹ
    bool GetShow(size_t index, PacketContent &content) const;

    //���չʾ����
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