#include <WinSock2.h>
#include <vector>
#include <common.h>
#include "protocol.h"
#include "packets.h"
#include "analysis.h"
#include "filter.h"
#include "view/view.h"
#include "colour.h"
#include "connect.h"
#include "FileCache.h"

//最大缓存封包数量
static ULONGLONG gs_llMaxCount = 22110000;

//解析之后的封包池
//vector<PPacketContent> g_filter_packets;

//符合显示过滤条件的封包池，如果显示条件重置，刷新此列表
//vector<PPacketContent> g_show_packets;

//网络链接展示的颜色
map<mstring/*网络链接标识*/, DWORD/*颜色rgb*/> s_connect_colours;

//需要释放的的packet列表
static vector<PPacketContent> s_clear_packets;
HANDLE s_clear_lock = CreateMutexA(NULL, FALSE, NULL);

#define  LOCK_CLEAR            WaitForSingleObject(s_clear_lock, INFINITE)
#define  UNLOCK_CLEAR    ReleaseMutex(s_clear_lock)

HANDLE g_filter_lock = CreateMutexA(NULL, FALSE, NULL);
HANDLE g_show_lock = CreateMutexA(NULL, FALSE, NULL);
static HANDLE s_notice_event = CreateEventA(NULL, FALSE, FALSE, NULL);
static HANDLE s_leave_event = CreateEventA(NULL, TRUE, FALSE, NULL);
static HANDLE s_clear_event = CreateEventA(NULL, FALSE, FALSE, NULL);
static HANDLE s_work_thread = NULL;
static HANDLE s_clear_thread =NULL;

static BOOL s_suspend = FALSE;

volatile LONG g_packet_count = 0;
volatile LONG g_filter_count = 0;
volatile LONG g_show_count = 0;

DWORD WINAPI PacketAnalysisThread(LPVOID p);
DWORD WINAPI PacketClearThread(LPVOID p);

VOID SetMaxPacketCount(ULONGLONG llMaxCount)
{
    gs_llMaxCount = llMaxCount;
}

ULONGLONG GetMaxPacketCount()
{
    return gs_llMaxCount;
}

VOID WINAPI NoticePacket()
{
    SetEvent(s_notice_event);
}

VOID WINAPI BeginWork()
{
    if (s_work_thread)
    {
        return;
    }
    s_work_thread = CreateThread(NULL, 0, PacketAnalysisThread, NULL, 0, NULL);
    s_clear_thread = CreateThread(NULL, 0, PacketClearThread, NULL, 0, NULL);
    Sleep(0);
}

VOID WINAPI EndWork()
{
    if (s_work_thread || s_clear_thread)
    {
        SetEvent(s_leave_event);
        if (WAIT_TIMEOUT == WaitForSingleObject(s_work_thread, 500))
        {
            TerminateThread(s_work_thread, 0);
        }
        CloseHandle(s_work_thread);

        if (WAIT_TIMEOUT == WaitForSingleObject(s_clear_thread, 500))
        {
            TerminateThread(s_clear_thread, 1000);
        }
        CloseHandle(s_clear_thread);
        ResetEvent(s_leave_event);
        s_work_thread = s_leave_event = NULL;
    }
}

VOID WINAPI GetPacketShowBuffer(IN const PPacketContent packet)
{
    bool mk = false;
    mstring tmp;

    if (em_tls_tcp == packet->m_tls_type)
    {
        packet->m_show = "Tcp FLAG[";
        if (packet->m_tls_header.m_tcp.is_flag_ack())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "ACK";
        }

        if (packet->m_tls_header.m_tcp.is_flag_fin())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "FIN";
        }

        if (packet->m_tls_header.m_tcp.is_flag_push())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "PUSH";
        }

        if (packet->m_tls_header.m_tcp.is_flag_rst())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "RST";
        }

        if (packet->m_tls_header.m_tcp.is_flag_syn())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "SYN";
        }

        if (packet->m_tls_header.m_tcp.is_flag_urg())
        {
            if (mk)
            {
                packet->m_show += " ";
            }
            mk = true;
            packet->m_show += "URG";
        }

        if (mk)
        {
            packet->m_show += "]";
        }
        else
        {
            packet->m_show.clear();
        }
        tmp.format("Seq=%u Ack=%u Win=%u CheckSum=%u", packet->m_tls_header.m_tcp.m_sequenceNumber, packet->m_tls_header.m_tcp.m_acknowledgeNumber, packet->m_tls_header.m_tcp.m_windows, packet->m_tls_header.m_tcp.m_checksum);
        if (packet->m_show.size() > 0)
        {
            packet->m_show += " ";
            packet->m_show += tmp;
        }
    }
    else if (em_tls_udp == packet->m_tls_type)
    {
        packet->m_show.format("Udp Length=%d CheckSum=%d", packet->m_tls_header.m_udp.m_len, packet->m_tls_header.m_udp.m_checksum);
    }
    else if (em_tls_icmp == packet->m_tls_type)
    {
        packet->m_show.format("Icmp Type=%d Code=%d CheckSum=%d Seq=%d Time=%d", packet->m_tls_header.m_icmp.m_type, packet->m_tls_header.m_icmp.m_code, packet->m_tls_header.m_icmp.m_checksum, packet->m_tls_header.m_icmp.m_sequence, packet->m_tls_header.m_icmp.m_timestAmp);
    }
    else
    {
        packet->m_show.format("IP Os=%d Id=%d TTL=%d Protocol=%d CheckSum=%d", packet->m_ip_header.m_ipTOS, packet->m_ip_header.m_ipID, packet->m_ip_header.m_ipTTL, packet->m_ip_header.m_ipProtocol, packet->m_ip_header.m_ipChecksum);
    }
}

bool AnalysisProtocol(IN OUT PacketContent &msg)
{
    if (msg.m_packet.size() < sizeof(IPHeader))
    {
        dp(L"buffer error");
        return false;
    }
    PIPHeader ip = (PIPHeader)msg.m_packet.c_str();
    int count = msg.m_packet.size();
    int length = ntohs(ip->m_ipLength);
    if (count != length)
    {
        dp(L"paket length error");
    }

    int itm = 0;
    itm += sizeof(IPHeader);

    bool stat = false;
    switch(ip->m_ipProtocol)
    {
    case IPPROTO_TCP:
        {
            if (msg.m_packet.size() - sizeof(IPHeader) < sizeof(TCPHeader))
            {
                dp(L"tcp buffer error");
                break;
            }
            TCPHeader *tcp_header = (TCPHeader *)(msg.m_packet.c_str() + sizeof(IPHeader));
            msg.m_tls_type = em_tls_tcp;
            msg.m_ip_header = *ip;
            msg.m_tls_header.m_tcp = *tcp_header;
            msg.m_tls_header.m_tcp.n2h();
            stat = true;
        }
        break;
    case  IPPROTO_UDP:
        {
            if (msg.m_packet.size() - sizeof(IPHeader) < sizeof(UDPHeader))
            {
                dp(L"udp buffer error");
                break;
            }
            PUDPHeader udp_header = (PUDPHeader)(msg.m_packet.c_str() + itm);
            msg.m_tls_type= em_tls_udp;
            msg.m_ip_header = *ip;
            msg.m_tls_header.m_udp = *udp_header;
            msg.m_tls_header.m_udp.n2h();
            stat = true;
        }
        break;
    case  IPPROTO_ICMP:
        {
            if (msg.m_packet.size() - sizeof(IPHeader) < sizeof(ICMPHeader))
            {
                dp(L"icmp buffer error");
                break;
            }
            PICMPHeader icmp_header = (PICMPHeader)(msg.m_packet.c_str() + itm);
            msg.m_tls_type = em_tls_icmp;
            msg.m_ip_header = *ip;
            msg.m_tls_header.m_icmp = *icmp_header;
            msg.m_tls_header.m_icmp.n2h();
            stat = true;
        }
        break;
    default:
        {
            dp(L"type error\n");
        }
        break;
    }
    return stat;
}

VOID WINAPI PacketAnalysis(IN OUT PacketContent &msg)
{
    InterlockedIncrement(&g_packet_count);
    BOOL bOverFLow = FALSE;
    do
    {
        {
            LOCK_FILTER;
            if (CFileCache::GetInst()->GetPacketCount() >= gs_llMaxCount)
            {
                UNLOCK_FILTER;
                bOverFLow = TRUE;
                break;
            }
            UNLOCK_FILTER;

            if(AnalysisProtocol(msg))
            {
                LOCK_FILTER;
                if (0 == msg.m_colour)
                {
                    if (s_connect_colours.end() == s_connect_colours.find(msg.m_packet_mark))
                    {
                        s_connect_colours[msg.m_packet_mark] = GetColourValue();
                    }
                    msg.m_colour = s_connect_colours[msg.m_packet_mark];
                }

                msg.m_ip_header.n2h();
                if (msg.m_ip_header.m_ipLength != msg.m_packet.size())
                {
                    dp(L"ip pakcet length error");
                }
                InterlockedIncrement(&g_filter_count);
                SYSTEMTIME time;
                GetLocalTime(&time);
                msg.m_time.format("%04d-%02d-%02d %02d:%02d:%02d %03d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

                bool show = false;
                if (IsPacketPassShow(&msg))
                {
                    InterlockedIncrement(&g_show_count);
                    GetPacketShowBuffer(&msg);
                    show = true;
                    if (IsWindow(g_main_view))
                    {
                        PostDelayMessage(g_main_view, MSG_UPDATE_DATA, 0, 0);
                    }
                }
                CFileCache::GetInst()->PushPacket(msg, show);
                UNLOCK_FILTER;
            }
        }
        UpdatePacketsCount();
    } while (FALSE);

    if (bOverFLow)
    {
        NotifyPacketsOverFlow();
    }
}

DWORD WINAPI PacketAnalysisThread(LPVOID p)
{
    HANDLE arry[] = {s_leave_event, s_notice_event};
    mstring packet;
    PacketContent msg;
    while(TRUE)
    {
        DWORD ret = WaitForMultipleObjects(sizeof(arry) / sizeof(HANDLE), arry, FALSE, INFINITE);
        if (WAIT_OBJECT_0 == ret)
        {
            break;
        }
        else if (WAIT_OBJECT_0 + 1 == ret)
        {
            packet.clear();
            while(PopPacket(packet))
            {
                msg.clear();
                msg.set_packet(packet);
                if (!TraverseWatchers(msg))
                {
                    continue;
                }
                if (!s_suspend)
                {
                    PacketAnalysis(msg);
                }
            }
        }
        else
        {
            dp(L"wait error");
        }
    }
    return 0;
}

DWORD WINAPI PacketClearThread(LPVOID p)
{
    HANDLE arry[] = {s_leave_event, s_clear_event};
    mstring packet;
    vector<PPacketContent> tmp;
    vector<PPacketContent>::iterator its;
    while(TRUE)
    {
        DWORD ret = WaitForMultipleObjects(sizeof(arry) / sizeof(HANDLE), arry, FALSE, INFINITE);
        if (WAIT_OBJECT_0 == ret)
        {
            break;
        }
        else if (WAIT_OBJECT_0 + 1 == ret)
        {
            tmp.clear();
            LOCK_CLEAR;
            tmp.resize(s_clear_packets.size());
            tmp = s_clear_packets;
            s_clear_packets.clear();
            s_clear_packets.swap(vector<PPacketContent>());
            UNLOCK_CLEAR;
            for (its = tmp.begin() ; its != tmp.end() ; its++)
            {
                delete *its;
            }
            tmp.clear();
            tmp.swap(vector<PPacketContent>());
        }
        else
        {
            dp(L"wait error");
        }
    }
    return 0;
}

VOID RecheckFilterPacket()
{
    LOCK_FILTER;
    CFileCache::GetInst()->ClearShow();

    InterlockedExchange(&g_show_count, 0);
    InterlockedExchange(&g_filter_count, 0);
    for (size_t i = 0 ; i < CFileCache::GetInst()->GetPacketCount() ; i++)
    {
        PacketContent packet;
        CFileCache::GetInst()->GetPacket(i, packet);

        if (IsPacketPassShow(&packet))
        {
            CFileCache::GetInst()->SetShowPacket(i);
        }
    }
    UNLOCK_FILTER;
    UpdatePacketsCount();
}

VOID RecheckShowPacket()
{
    LOCK_FILTER;
    CFileCache::GetInst()->ClearShow();
    for (size_t i = 0 ; i < CFileCache::GetInst()->GetPacketCount() ; i++)
    {
        PacketContent content;
        CFileCache::GetInst()->GetPacket(i, content);
        if (IsPacketPassShow(&content))
        {
            CFileCache::GetInst()->SetShowPacket(i);
        }
    }
    UNLOCK_FILTER;
    UpdatePacketsCount();
}

VOID ClearPackets()
{
    //清除封包缓存
    ClearPacketBuffer();
    //清除http缓存
    ClearHttpBuffer();
    //清除connect缓存
    ClearConnect();
    LOCK_FILTER;
    s_connect_colours.clear();
    ClearColour();
    LOCK_CLEAR;
    //s_clear_packets.insert(s_clear_packets.end(), g_filter_packets.begin(), g_filter_packets.end());
    UNLOCK_CLEAR;
    //SetEvent(s_clear_event);
    //g_filter_packets.clear();
    //g_show_packets.clear();
    //g_filter_packets.swap(vector<PPacketContent>());
    //g_show_packets.swap(vector<PPacketContent>());
    CFileCache::GetInst()->ClearCache();
    UNLOCK_FILTER;
    InterlockedExchange(&g_packet_count, 0);
    InterlockedExchange(&g_filter_count, 0);
    InterlockedExchange(&g_show_count, 0);
    UpdatePacketsCount();
}

VOID SuspendSniffer()
{
    s_suspend = TRUE;
}

VOID RestSniffer()
{
    s_suspend = FALSE;
}

BOOL IsSnifferSuspend()
{
    return s_suspend;
}