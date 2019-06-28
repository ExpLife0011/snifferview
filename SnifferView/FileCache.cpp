#include <WinSock2.h>
#include <Shlwapi.h>
#include "common/tpool.h"
#include "FileCache.h"
#include "global.h"

#define  CACHE_DATA_MARK        ("\a\a")

/*
2019-06-14 18:02
之前的转码有严重的逻辑漏洞:
用\n\n进行节点的分割，为了防止之前的字符串中已经存在\n\n将
字符串中的\n替换成#\n,这里忽略了一种特殊情况比如字符串结尾是\n,
即使将其替换成#\n也会对分割产生干扰，导致边界移位造成连锁反应，
导致解析错误正确的方式是转码时两边都进行转义处理。
*/
CFileCache *CFileCache::GetInst() {
    static CFileCache *sPtr = NULL;

    if (NULL == sPtr)
    {
        sPtr = new CFileCache();
    }
    return sPtr;
}

CFileCache::CFileCache() {
}

CFileCache::~CFileCache() {
}

void CFileCache::Encode(mstring &str) const {
    str.repsub("\a", "@\a@");
}

void CFileCache::Decode(mstring &str)  const {
    str.repsub("@\a@", "\a");
}

bool CFileCache::InitFileCache() {
    char dbPath[256];

    GetModuleFileNameA(NULL, dbPath, 256);
    PathAppendA(dbPath, "..\\SniffCache.db");

    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesA(dbPath))
    {
        DeleteFileA(dbPath);
    }

    CScopedLocker locker(&mCacheLocker);
    mCacheFile = dbPath;
    mWriteHandle = CreateFileA(
        dbPath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
        NULL
        );
    mMaxCache = 220000;
    return true;
}

mstring CFileCache::GetContent(const PacketPosInFile &pos) const {
    HANDLE hRead = CreateFileA(
        mCacheFile.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    int d = GetLastError();

    if (INVALID_HANDLE_VALUE == hRead)
    {
        return "";
    }

    mstring result;
    SetFilePointer(hRead, pos.mStartPos, NULL, FILE_BEGIN);

    char buff[4096];
    DWORD bufSize = 4096;
    DWORD count = 0;
    DWORD readCount = pos.mEndPos - pos.mStartPos;
    DWORD curRead = 0;
    while (true) {
        curRead = (readCount > bufSize ? bufSize : readCount);

        DWORD tmp = 0;
        if (!ReadFile(hRead, buff, readCount, &tmp, NULL) || 0 == tmp)
        {
            break;
        }

        result.append(buff, tmp);
        readCount -= tmp;

        if (0 == readCount)
        {
            break;
        }
    }
    CloseHandle(hRead);
    return result;
}

bool CFileCache::GetPacketFromStr(const std::mstring &str, PacketContent &packet) const {
    int sessionIndex = 0;
    size_t lastPos = 0;

    size_t pos = 0;
    mstring session;
    static int sStrLength = lstrlenA(CACHE_DATA_MARK);

    while (true) {
        pos = str.find(CACHE_DATA_MARK, lastPos);
        if (mstring::npos == pos)
        {
            break;
        }

        session = str.substr(lastPos, pos - lastPos);
        Decode(session);
        lastPos = pos + sStrLength;
        switch (sessionIndex) {
                case 0:
                    {
                        memcpy(&packet.m_packet_init, session.c_str(), session.size());
                    }
                    break;
                case 1:
                    {
                        packet.m_packet_mark = session;
                    }
                    break;
                case 2:
                    {
                        packet.m_dec_mark = session;
                    }
                    break;
                case 3:
                    {
                        memcpy(&packet.m_ip_header, session.c_str(), session.size());
                    }
                    break;
                case 4:
                    {
                        memcpy(&packet.m_tls_type, session.c_str(), session.size());

                        if (packet.m_tls_type > 3)
                        {
                            int dd = 1234;
                        }
                    }
                    break;
                case 5:
                    {
                        memcpy(&packet.m_tls_header, session.c_str(), session.size());
                    }
                    break;
                case 6:
                    {
                        memcpy(&packet.m_user_type, session.c_str(), session.size());
                    }
                    break;
                case 7:
                    {
                        memcpy(&packet.m_user_content, session.c_str(), session.size());
                    }
                    break;
                case 8:
                    //url list
                    {
                        int size = 0;
                        memcpy(&size, session.c_str(), session.size());

                        size_t pos0 = lastPos;
                        size_t pos1 = 0;
                        mstring tmp2;
                        for (int j = 0 ; j < size ; j++)
                        {
                            pos1 = str.find(CACHE_DATA_MARK, pos0);
                            tmp2 = str.substr(pos0, pos1 - pos0);
                            tmp2.repsub("#\n", "\n");
                            packet.m_urls.push_back(tmp2);
                            pos0 = (pos1 + sStrLength);
                        }
                        lastPos += (pos0 - lastPos);
                    }
                    break;
                case 9:
                    {
                        packet.m_packet = session;
                    }
                    break;
                case 10:
                    {
                        packet.m_show = session;
                    }
                    break;
                case 11:
                    {
                        packet.m_time = session;
                    }
                    break;
                case  12:
                    {
                        memcpy(&packet.m_colour, session.c_str(), session.size());
                    }
                    break;
                default:
                    {
                        int dd = 1234;
                    }
                    break;
        }
        sessionIndex++;
    }
    return true;
}

size_t CFileCache::GetPacketCount() const {
    CScopedLocker locker(&mCacheLocker);
    return mPacketSet.size();
}

bool CFileCache::GetPacket(size_t index, PacketContent &content) const {
    CScopedLocker locker(&mCacheLocker);
    PacketPosInFile pos;
    {
        if (index >= mPacketSet.size())
        {
            return false;
        }

        pos = mPacketSet[index];
    }

    if (pos.mInCache)
    {
        content =  *pos.mPtr;
        return true;
    } else {
        return GetPacketFromStr(GetContent(pos), content);
    }
}

bool CFileCache::EnumPacket(pfnPacketEnumHandler pfnHandler, void *param) {
    CScopedLocker locker(&mCacheLocker);
    int index = 0;
    PacketContent tmp;
    for (vector<PacketPosInFile>::const_iterator it = mPacketSet.begin() ; it != mPacketSet.end() ; it++, index++)
    {
        bool loop = true;
        if (it->mInCache)
        {
            loop = pfnHandler(index, it->mPtr, param);
        } else {
            GetPacketFromStr(GetContent(*it), tmp);
            loop = pfnHandler(index, &tmp, param);
        }

        if (!loop)
        {
            return true;
        }
    }
    return true;
}

size_t CFileCache::GetShowCount() const {
    CScopedLocker locker(&mCacheLocker);
    return mShowSet.size();
}

bool CFileCache::GetShow(size_t index, PacketContent &packet) const {
    CScopedLocker locker(&mCacheLocker);
    PacketPosInFile pos;
    {
        if (index >= mShowSet.size())
        {
            return false;
        }
        pos = mShowSet[index];
    }

    if (pos.mInCache)
    {
        packet = *pos.mPtr;
    } else {
        GetPacketFromStr(GetContent(pos), packet);
    }
    return true;
}

void CFileCache::SetShowPacket(size_t index) {
    CScopedLocker locker(&mCacheLocker);
    /*
    基于效率考虑,优先将数据插入缓存尾部,因为大部分情况属于这种.
    如果不满足要求,可能是随机插入情况,通过二分查找找到合适的位置
    将新的节点插入合适的位置,集合是按照偏移位置升序排列的。
    */
    if (index >= mPacketSet.size())
    {
        return;
    }

    PacketPosInFile pos = mPacketSet[index];
    size_t showSize = mShowSet.size();
    if ((0 == showSize) || (pos.mStartPos >= mShowSet[showSize - 1].mEndPos))
    {
        mShowSet.push_back(pos);
    } else {
        size_t pos0 = 0, pos1 = (mShowSet.size() - 1);
        while (true) {
            if (pos1 == pos0)
            {
                if (pos.mStartPos == mShowSet[pos0].mStartPos)
                {
                    break;
                }

                if (pos.mStartPos < mShowSet[pos0].mStartPos)
                {
                    mShowSet.insert(mShowSet.begin() + pos0, pos);
                }
                else{
                    mShowSet.insert(mShowSet.begin() + pos0 + 1, pos);
                }
                break;
            } else if (pos1 == (pos0 + 1)) {
                //需要特殊处理，否则会出现pos0 + pos1 / 2 == pos0的情况
                if ((pos.mStartPos == mShowSet[pos0].mStartPos) || (pos.mStartPos == mShowSet[pos1].mStartPos))
                {
                    break;
                }

                if (pos.mStartPos < mShowSet[pos0].mStartPos)
                {
                    mShowSet.insert(mShowSet.begin() + pos0, pos);
                } else if (pos.mStartPos < mShowSet[pos1].mStartPos)
                {
                    mShowSet.insert(mShowSet.begin() + pos1, pos);
                } else {
                    mShowSet.insert(mShowSet.begin() + pos1 + 1, pos);
                }
                break;
            }

            size_t mid = (pos1 + pos0) / 2;
            PacketPosInFile tmp = mShowSet[mid];

            if (tmp.mStartPos == pos.mStartPos)
            {
                break;
            }

            if (tmp.mStartPos > pos.mStartPos)
            {
                pos1 = mid;
            } else {
                pos0 = mid;
            }
        }
    }
}

void CFileCache::ClearShow() {
    CScopedLocker locker(&mCacheLocker);
    mShowSet.clear();
}

void CFileCache::ClearCache() {
    CScopedLocker locker1(&mCacheLocker);
    if (INVALID_HANDLE_VALUE != mWriteHandle)
    {
        {
            CloseHandle(mWriteHandle);
            mWriteHandle = NULL;
            DeleteFileA(mCacheFile.c_str());

            mWriteHandle = CreateFileA(
                mCacheFile.c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
                NULL
                );
        }

        {
            //线程中释放动态申请的缓存
            class CCacheClearTask : public ThreadRunable {
            public:
                CCacheClearTask(const list<PPacketContent> &set1) {
                    mSet = set1;
                }

                void run() {
                    for (list<PPacketContent>::const_iterator it = mSet.begin() ; it != mSet.end() ; it++)
                    {
                        delete *it;
                    }
                }
            private:
                list<PPacketContent> mSet;
            };

            CScopedLocker locker2(&mCacheLocker);
            dp(L"test1");
            DWORD t1 = GetTickCount();
            list<PPacketContent> set1;
            for (vector<PacketPosInFile>::const_iterator ij = mPacketSet.begin() ; ij != mPacketSet.end() ; ij++)
            {
                if (ij->mInCache == true)
                {
                    set1.push_back(ij->mPtr);
                }
            }

            dp(L"test2:%d", (GetTickCount() - t1));
            t1 = GetTickCount();

            gThreadPool->exec(new CCacheClearTask(set1));
            dp(L"test3:%d", (GetTickCount() - t1));
            mShowSet.clear();
            mPacketSet.clear();
        }
    }
}

void CFileCache::PushToFile(const PacketContent &packet, PacketPosInFile &pos) {
    mstring content;
    mstring tmp;
    tmp.append((const char *)&packet.m_packet_init, sizeof(packet.m_packet_init));
    Encode(tmp);

    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp = packet.m_packet_mark;
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp = packet.m_dec_mark;
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_ip_header, sizeof(packet.m_ip_header));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_tls_type, sizeof(packet.m_tls_type));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_tls_header, sizeof(packet.m_tls_header));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_user_type, sizeof(packet.m_user_type));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_user_content, sizeof(packet.m_user_content));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    int urlCount = packet.m_urls.size();
    tmp.clear();
    tmp.append((const char *)&urlCount, sizeof(urlCount));
    Encode(tmp);;
    content += tmp;
    content += CACHE_DATA_MARK;

    for (list<mstring>::const_iterator it = packet.m_urls.begin() ; it != packet.m_urls.end() ; it++)
    {
        tmp.clear();
        tmp = *it;
        Encode(tmp);
        content += tmp;
        content += CACHE_DATA_MARK;
    }
    tmp.clear();
    tmp = packet.m_packet; 
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp = packet.m_show;
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp = packet.m_time;
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    tmp.clear();
    tmp.append((const char *)&packet.m_colour, sizeof(packet.m_colour));
    Encode(tmp);
    content += tmp;
    content += CACHE_DATA_MARK;

    DWORD lowSize = 0, highSize = 0;
    {
        CScopedLocker locker1(&mCacheLocker);
        lowSize = GetFileSize(mWriteHandle, &highSize);
        SetFilePointer(mWriteHandle, 0, NULL, FILE_END);

        pos.mStartPos = lowSize;
        pos.mEndPos = lowSize + content.size();

        DWORD dw = 0;
        WriteFile(mWriteHandle, content.c_str(), content.size(), &dw, NULL);
        FlushFileBuffers(mWriteHandle);
    }
}

void CFileCache::PushPacket(const PacketContent &packet, bool show) {
    PacketPosInFile cache;
    if (mPacketSet.size() < mMaxCache)
    {
        cache.mInCache = true;
        cache.mPtr = new PacketContent(packet);
    } else {
        cache.mInCache = false;
        PushToFile(packet, cache);
    }

    {
        CScopedLocker locker2(&mCacheLocker);
        if (show)
        {
            mShowSet.push_back(cache);
        }
        mPacketSet.push_back(cache);
    }
}