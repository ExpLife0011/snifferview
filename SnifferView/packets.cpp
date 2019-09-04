#include <Windows.h>
#include <list>
#include <vector>
#include "../ComLib/mstring.h"

using namespace std;

HANDLE s_packets_lock = CreateMutexA(NULL, FALSE, NULL);

//·â°ü»º³å³Ø
mstring s_org_packets;
list<size_t> s_packet_border;

static void _LockPacks()
{
    WaitForSingleObject(s_packets_lock, INFINITE);
}

static void _UnLockPackets()
{
    ReleaseMutex(s_packets_lock);
}

VOID WINAPI ClearPacketBuffer()
{
    _LockPacks();
    s_org_packets.clear_with_mem();
    s_packet_border.clear();
    _UnLockPackets();
}

VOID WINAPI PushPacket(const char *buffer, size_t length)
{
    _LockPacks();
    s_org_packets.append(buffer, length);
    if (s_packet_border.size() > 0)
    {
        s_packet_border.push_back((s_packet_border.back())+ length); 
    }
    else
    {
        s_packet_border.push_back(0);
        s_packet_border.push_back(length);
    }
    _UnLockPackets();
}

BOOL WINAPI PopPacket(mstring &packet)
{
    _LockPacks();
    if (s_packet_border.size() <= 1)
    {
        s_packet_border.clear();
        s_org_packets.clear();
        _UnLockPackets();
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
    _UnLockPackets();
    return TRUE;
}