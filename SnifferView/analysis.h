#pragma  once
#include <vector>
#include <mstring.h>
#include "protocol.h"
#include "packets.h"
#include "http.h"

using namespace std;

//�����Э������
enum TransferProtocolType
{
    em_tls_none = 0,
    em_tls_tcp,
    em_tls_udp,
    em_tls_icmp
};

//�����Э��ͷ
union TransferProtocolHeader
{
    TCPHeader m_tcp;
    UDPHeader m_udp;
    ICMPHeader m_icmp;
};

//Ӧ�ò�Э������
enum UserProtocolType
{
    em_user_none = 0,
    em_user_http,
    em_user_smtp,
    em_user_ftp
};

//Ӧ�ò�Э������
union UserProtocol
{
    HttpPacket m_http;
};

#define        FLAG_SYN    (1 << 0)
#define        FLAG_FIN    (1 << 1)

//���syn��fin��ʶ����ʶ���Ƿ���ͬһ������
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

//������ݼ���������
typedef struct PacketContent
{
    bool m_packet_init;                         //��������Ƿ��Ѿ���ʼ��
    mstring m_packet_mark;                      //�����Ψһ��ʶ
    mstring m_dec_mark;                         //������ķ��Ψһ��ʶ����ǰֻӦ����tcp������http�Ľ���

    IPHeader m_ip_header;                       //���ipͷ

    TransferProtocolType m_tls_type;            //�����Э������
    TransferProtocolHeader m_tls_header;        //�����Э��ͷ

    UserProtocolType m_user_type;               //Ӧ�ò�Э������
    UserProtocol m_user_content;                //Ӧ�ò�Э������
    list<mstring> m_urls;                       //httpЭ���url�б������ж��url

    mstring m_packet;                           //�������
    mstring m_show;                             //���չʾ����
    mstring m_time;                             //���յ������ʱ��

    DWORD m_colour;                             //չʾ����ɫ��ɫ

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