#include <Windows.h>
#include <list>
#include <vector>
#include "../ComLib/mstring.h"
#include "../ComLib/common.h"
#include "../ComLib/LockBase.h"
#include "analysis.h"

using namespace std;

static CCriticalSectionLockable *gsPacketLocker = NULL;

//封包缓冲池
//缓冲区长度,调试用
static ULONGLONG sBuffSize = 0;
mstring s_org_packets;
list<size_t> s_packet_border;

VOID ClearPacketBuffer()
{
    CScopedLocker locker(gsPacketLocker);
    s_org_packets.clear_with_mem();
    s_packet_border.clear();
}

VOID PushPacket(const char *buffer, size_t length)
{
    CScopedLocker locker(gsPacketLocker);
    try {
        if (IsSnifferSuspend())
        {
            return;
        }

        s_org_packets.append(buffer, length);
        if (s_packet_border.size() > 0)
        {
            s_packet_border.push_back((s_packet_border.back()) + length); 
        }
        else
        {
            s_packet_border.push_back(0);
            s_packet_border.push_back(length);
        }

        static DWORD sLastCount = GetTickCount();
        if (GetTickCount() - sLastCount >= 5000)
        {
            sLastCount = GetTickCount();
            dp(L"bufferSize:%u", s_org_packets.size());
        }
    } catch (exception &e){
        ClearPacketBuffer();
        dp(L"append err, buff size1:%I64d size2:%d, buffer:0x%p, length:%d, msg:%hs", sBuffSize, (int)s_packet_border.size(), buffer, length, e.what());
    }
}

BOOL PopPacket(mstring &packet)
{
    CScopedLocker locker(gsPacketLocker);
    try {
        if (s_packet_border.size() <= 1)
        {
            s_packet_border.clear();
            s_org_packets.clear();
            return FALSE;
        }
        list<size_t>::iterator itm = s_packet_border.begin();
        size_t b = *itm;
        itm++;
        size_t e = *itm;
        packet.assign(s_org_packets, b, e - b);
        s_packet_border.pop_front();
        if (1 == s_packet_border.size())
        {
            s_packet_border.clear();
            s_org_packets.clear();
        }
    } catch (std::exception &e) {
        ClearPacketBuffer();
        dp(L"pop packet exception,msg:%hs", e.what());
        return FALSE;
    }
    return TRUE;
}

void InitPacketLocker() {
    gsPacketLocker = new CCriticalSectionLockable();
}