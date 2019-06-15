#include <WinSock2.h>
#include <Shlwapi.h>
#include "FileCache.h"

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

    CScopedLocker locker(&mDataLocker);
    mCacheFile = dbPath;
    mFileHandle = CreateFileA(
        dbPath,
        FILE_ALL_ACCESS,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
        NULL
        );
    return true;
}

mstring CFileCache::GetContent(const PacketPosInFile &pos) const {
    CScopedLocker locker(&mDataLocker);
    mstring result;
    SetFilePointer(mFileHandle, pos.mStartPos, NULL, FILE_BEGIN);

    char buff[4096];
    DWORD bufSize = 4096;
    DWORD count = 0;
    DWORD readCount = pos.mEndPos - pos.mStartPos;
    DWORD curRead = 0;
    while (true) {
        curRead = (readCount > bufSize ? bufSize : readCount);

        DWORD tmp = 0;
        if (!ReadFile(mFileHandle, buff, readCount, &tmp, NULL) || 0 == tmp)
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
    CScopedLocker locker(&mDataLocker);
    return mPacketSet.size();
}

bool CFileCache::GetPacket(size_t index, PacketContent &packet) const {
    CScopedLocker locker(&mDataLocker);
    if (index >= mPacketSet.size())
    {
        return false;
    }

    PacketPosInFile pos = mPacketSet[index];
    mstring content = GetContent(pos);
    return true;
}

size_t CFileCache::GetShowCount() const {
    CScopedLocker locker(&mDataLocker);
    return mShowSet.size();
}

bool CFileCache::GetShow(size_t index, PacketContent &packet) const {
    CScopedLocker locker(&mDataLocker);
    if (index >= mShowSet.size())
    {
        return false;
    }

    mstring content = GetContent(mShowSet[index]);
    bool ret = GetPacketFromStr(content, packet);
    return ret;
}

void CFileCache::SetShowPacket(size_t index) {
    CScopedLocker locker(&mDataLocker);
}

void CFileCache::ClearShow() {
   CScopedLocker locker(&mDataLocker);
    mShowSet.clear();
}

void CFileCache::ClearCache() {
    if (INVALID_HANDLE_VALUE != mFileHandle)
    {
        CScopedLocker locker(&mDataLocker);
        CloseHandle(mFileHandle);
        mFileHandle = NULL;
        DeleteFileA(mCacheFile.c_str());
        mShowSet.clear();
        mPacketSet.clear();

        mFileHandle = CreateFileA(
            mCacheFile.c_str(),
            FILE_ALL_ACCESS,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
            NULL
            );
    }
}

void CFileCache::PushPacket(const PacketContent &packet, bool show) {
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

    CScopedLocker locker(&mDataLocker);
    lowSize = GetFileSize(mFileHandle, &highSize);
    SetFilePointer(mFileHandle, 0, NULL, FILE_END);

    PacketPosInFile pos;
    pos.mStartPos = lowSize;
    pos.mEndPos = lowSize + content.size();

    DWORD dw = 0;
    WriteFile(mFileHandle, content.c_str(), content.size(), &dw, NULL);
    FlushFileBuffers(mFileHandle);

    if (show)
    {
        mShowSet.push_back(pos);
    }
    mPacketSet.push_back(pos);
}