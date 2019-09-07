#include <WinSock2.h>
#include <vector>
#include "../ComLib/common.h"
#include "protocol.h"
#include "packets.h"
#include "analysis.h"
#include "filter.h"
#include "view/view.h"
#include "FileCache/FileCache.h"
#include "PacketCache.h"

//最大缓存封包数量
static ULONGLONG gs_llMaxCount = 22110000;

//HANDLE g_filter_lock = CreateMutexA(NULL, FALSE, NULL);
//HANDLE g_show_lock = CreateMutexA(NULL, FALSE, NULL);
static HANDLE s_notice_event = CreateEventA(NULL, FALSE, FALSE, NULL);
static HANDLE s_leave_event = CreateEventA(NULL, TRUE, FALSE, NULL);
static HANDLE s_work_thread = NULL;

static BOOL s_suspend = FALSE;
DWORD WINAPI PacketAnalysisThread(LPVOID p);

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

    Sleep(0);
}

VOID WINAPI EndWork()
{
    if (s_work_thread)
    {
        SetEvent(s_leave_event);
        if (WAIT_TIMEOUT == WaitForSingleObject(s_work_thread, 500))
        {
            TerminateThread(s_work_thread, 0);
        }
        CloseHandle(s_work_thread);
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

VOID WINAPI PacketAnalysis(IN OUT PacketContent &msg)
{
    BOOL bOverFLow = FALSE;
    do
    {
        {
            if (CFileCache::GetInst()->GetPacketCount() >= gs_llMaxCount)
            {
                bOverFLow = TRUE;
                break;
            }

            SYSTEMTIME time;
            GetLocalTime(&time);
            msg.m_time.format("%04d-%02d-%02d %02d:%02d:%02d %03d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

            bool show = false;
            if (IsPacketPassShow(&msg))
            {
                GetPacketShowBuffer(&msg);
                show = true;
                if (IsWindow(g_main_view))
                {
                    PostDelayMessage(g_main_view, MSG_UPDATE_DATA, 0, 0);
                }
            }
            msg.m_ip_header.n2h();
            CFileCache::GetInst()->PushPacket(msg, show);
        }
        UpdatePacketsCount();
    } while (FALSE);

    if (!IsSnifferSuspend() && bOverFLow)
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
                    dp(L"TraverseWatchers Err");
                    continue;
                }
                PacketAnalysis(msg);
            }
        }
        else
        {
            dp(L"wait error");
        }
    }
    return 0;
}

static bool _PacketEnumHandler(size_t index, const PacketContent *info, void *param) {
    if (IsPacketPassShow((PacketContent *)&info))
    {
        CFileCache::GetInst()->SetShowPacket(index);
    }
    return true;
}

VOID RecheckShowPacket()
{
    CFileCache::GetInst()->ClearShow();
    CFileCache::GetInst()->EnumPacket(_PacketEnumHandler);

    UpdatePacketsCount();
}

VOID ClearPackets()
{
    //清除封包缓存
    ClearPacketBuffer();
    //清除http缓存
    ClearHttpBuffer();
    //清除connect缓存
    CPacketCacheMgr::GetInst()->ResetCacheMgr();
    CFileCache::GetInst()->ClearCache();
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