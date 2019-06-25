#pragma  once
#include <vector>
#include <mstring.h>
#include "protocol.h"
#include "packets.h"
#include "http.h"

using namespace std;

//传输层协议类型
enum TransferProtocolType
{
    em_tls_none = 0,
    em_tls_tcp,
    em_tls_udp,
    em_tls_icmp
};

//传输层协议头
union TransferProtocolHeader
{
    TCPHeader m_tcp;
    UDPHeader m_udp;
    ICMPHeader m_icmp;
};

//应用层协议类型
enum UserProtocolType
{
    em_user_none = 0,
    em_user_http,
    em_user_smtp,
    em_user_ftp
};

//应用层协议内容
union UserProtocol
{
    HttpPacket m_http;
};

#define        FLAG_SYN    (1 << 0)
#define        FLAG_FIN    (1 << 1)

//配合syn和fin标识用于识别是否是同一个链接
struct ConnectMark
{
    ULONG m_addr1;
    USHORT m_port1;
    ULONG m_addr2;
    USHORT m_port2;
    mstring m_mark;

    ConnectMark()
    {
        m_addr1 = m_addr2 = 0;
        m_port1 = m_port2 = 0;
    }

    bool operator < (const ConnectMark &ms)const
    {
        mstring testa;
        testa.format("%08x%04x%08x%04x%hs", m_addr1, m_port1, m_addr2, m_port2, m_mark.c_str());
        mstring testb;
        testb.format("%08x%04x%08x%04x%hs", ms.m_addr1, ms.m_port1, ms.m_addr2, ms.m_port2, ms.m_mark.c_str());
        return testa < testb;
    }

    bool operator > (const ConnectMark &ms)const
    {
        mstring testa;
        testa.format("%08x%04x%08x%04x%hs", m_addr1, m_port1, m_addr2, m_port2, m_mark.c_str());
        mstring testb;
        testb.format("%08x%04x%08x%04x%hs", ms.m_addr1, ms.m_port1, ms.m_addr2, ms.m_port2, ms.m_mark.c_str());
        return testa > testb;
    }

    bool operator == (const ConnectMark &ms) const
    {
        mstring testa;
        testa.format("%08x%04x%08x%04x%hs", m_addr1, m_port1, m_addr2, m_port2, m_mark.c_str());
        mstring testb;
        testb.format("%08x%04x%08x%04x%hs", ms.m_addr1, ms.m_port1, ms.m_addr2, ms.m_port2, ms.m_mark.c_str());
        return testa == testb;
    }
};

//封包内容及附加类型
typedef struct PacketContent
{
    bool m_packet_init;                         //封包内容是否已经初始化
    mstring m_packet_mark;                      //封包的唯一标识
    mstring m_dec_mark;                         //带方向的封包唯一标识，当前只应用与tcp，用于http的解析

    IPHeader m_ip_header;                       //封包ip头

    TransferProtocolType m_tls_type;            //传输层协议类型
    TransferProtocolHeader m_tls_header;        //传输层协议头

    UserProtocolType m_user_type;               //应用层协议类型
    UserProtocol m_user_content;                //应用层协议内容
    list<mstring> m_urls;                       //http协议的url列表，可能有多个url

    mstring m_packet;                           //封包内容
    mstring m_show;                             //封包展示内容
    mstring m_time;                             //接收到封包的时间

    DWORD m_colour;                             //展示背景色颜色

    void init()
    {
        m_packet_init = false;
        memset(&m_ip_header, 0x00, sizeof(m_ip_header));
        m_tls_type = em_tls_none;
        m_user_type = em_user_none;
        m_user_content.m_http.m_http_type = HTTP_STATE_UNINIT;
        m_colour = 0;
    }

    PacketContent()
    {
        init();
    }

    PacketContent(const mstring &packet)
    {
        init();
        m_packet.append(packet.c_str(), packet.size());
    }

    PacketContent(IN const PacketContent &ms)
    {
        init();
        *this = ms;
    }

    virtual ~PacketContent()
    {
    }

    void clear()
    {
        init();
        m_packet.clear_with_mem();
        m_packet_mark.clear_with_mem();
        m_dec_mark.clear_with_mem();
        m_packet.clear_with_mem();
        m_show.clear_with_mem();
        m_time.clear_with_mem();
    }

    void set_packet(const mstring &packet)
    {
        m_packet.clear();
        m_packet.append(packet.c_str(), packet.size());
    }
}PacketContent, *PPacketContent;

extern HANDLE g_filter_lock;
extern HANDLE g_show_lock;

#define  LOCK_FILTER      (WaitForSingleObject(g_filter_lock, INFINITE))
#define  UNLOCK_FILTER    (ReleaseMutex(g_filter_lock))

VOID WINAPI NoticePacket();

VOID WINAPI BeginWork();

VOID WINAPI EndWork();

VOID RecheckFilterPacket();

VOID RecheckShowPacket();

VOID ClearPackets();

VOID WINAPI GetPacketShowBuffer(IN const PPacketContent packet);

bool AnalysisProtocol(IN OUT PacketContent &msg);

VOID SuspendSniffer();

VOID RestSniffer();

BOOL IsSnifferSuspend();

ULONGLONG GetMaxPacketCount();

VOID SetMaxPacketCount(ULONGLONG llMaxCount);