#include <WinSock2.h>
#include <Windows.h>
#include "common/crc32.h"
#include "common/StrUtil.h"
#include "PacketUnique.h"
#include "protocol.h"
#include "analysis.h"

using namespace std;

CPacketUnique *CPacketUnique::GetInst() {
    static CPacketUnique *sPtr = NULL;

    if (NULL == sPtr)
    {
        sPtr = new CPacketUnique();
    }
    return sPtr;
}

bool CPacketUnique::GetUnique(PacketContent &packet) const {
    if (packet.m_tls_type == em_tls_tcp)
    {
        packet.m_packet_mark = GetTcpUnique(packet);
        packet.m_dec_mark = GetTcpUniqueWithDirection(packet);
        return true;
    } else if (packet.m_tls_type == em_tls_udp)
    {
        packet.m_packet_mark = GetUdpUnique(packet);
        return true;
    } else if (packet.m_tls_type == em_tls_icmp)
    {
        packet.m_packet_mark = GetIcmpUnique(packet);
        return true;
    } else {
        return false;
    }
}

mstring CPacketUnique::GetUniqueWithDirection(const PacketContent &packet) const {
    return GetTcpUniqueWithDirection(packet);
}

CPacketUnique::CPacketUnique() {
}

CPacketUnique::~CPacketUnique() {
}

bool CPacketUnique::ParserProtocol(PacketContent &msg) const {
    if (msg.m_packet.size() < sizeof(IPHeader))
    {
        dp(L"buffer error");
        return FALSE;
    }
    PIPHeader ip = (PIPHeader)msg.m_packet.c_str();
    int count = msg.m_packet.size();
    int length = ntohs(ip->m_ipLength);
    if (count != length)
    {
        dp(L"paket length error");
        return FALSE;
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

//packet init, protocol parser, unique create
BOOL CPacketUnique::PacketInit(IN OUT PacketContent &msg) {
    if (!GetInst()->ParserProtocol(msg))
    {
        return FALSE;
    }

    msg.m_packet_init = true;
    return GetInst()->GetUnique(msg);
}

mstring CPacketUnique::GetUdpUnique(const PacketContent &packet) const {
    mstring mark = "udp";
    DWORD addr1 = 0, addr2 = 0;
    unsigned int port1 = 0, port2 = 0;
    DWORD ipSrc = packet.m_ip_header.m_ipSource;
    DWORD ipDst = packet.m_ip_header.m_ipDestination;
    unsigned short portSrc = packet.m_tls_header.m_udp.m_sourcePort;
    unsigned short portDst = packet.m_tls_header.m_udp.m_destinationPort;

    if (ipSrc < ipDst)
    {
        addr1 = ipSrc, port1 = portSrc;
        addr2 = ipDst, port2 = portDst;
    }
    else if (ipSrc == ipDst)
    {
        if (portSrc < portDst)
        {
            addr1 = ipSrc;
            port1 = portSrc;
            addr2 = ipDst;
            port2 = portDst;
        }
        else
        {
            addr1 = ipDst;
            port1 = portDst;
            addr2 = ipSrc;
            port2 = portSrc;
        }
    }
    else
    {
        addr1 = ipDst;
        port1 = portDst;
        addr2 = ipSrc;
        port2 = portSrc;
    }

    mstring tmp = FormatA("%08x_%d-%08x_%d_%hs", addr1, port1, addr2, port2, mark.c_str());
    unsigned long magic = crc32(tmp.c_str(), tmp.size(), 0xff1234aa);

    //为提高查找效率将crc32校验前置
    return FormatA("%08x_%hs", magic, tmp.c_str());
}

mstring CPacketUnique::GetTcpUnique(const PacketContent &packet) const {
    mstring mark = "tcp";
    DWORD ipSrc = packet.m_ip_header.m_ipSource;
    DWORD ipDst = packet.m_ip_header.m_ipDestination;
    unsigned short portSrc = packet.m_tls_header.m_tcp.m_sourcePort;
    unsigned short portDst = packet.m_tls_header.m_tcp.m_destinationPort;
    DWORD addr1 = 0, addr2 = 0;
    unsigned int port1 = 0, port2 = 0;

    if (ipSrc < ipDst)
    {
        addr1 = ipSrc;
        port1 = portSrc;
        addr2 = ipDst;
        port2 = portDst;
    }
    else if (ipSrc == ipDst)
    {
        if (portSrc < portDst)
        {
            addr1 = ipSrc;
            port1 = portSrc;
            addr2 = ipDst;
            port2 = portDst;
        }
        else
        {
            addr1 = ipDst;
            port1 = portDst;
            addr2 = ipSrc;
            port2 = portSrc;
        }
    }
    else
    {
        addr1 = ipDst;
        port1 = portDst;
        addr2 = ipSrc;
        port2 = portSrc;
    }

    mstring tmp = FormatA("%08x_%d-%08x_%d_%hs", addr1, port1, addr2, port2, mark.c_str());
    unsigned long magic = crc32(tmp.c_str(), tmp.size(), 0xff1234aa);

    //为提高查找效率将crc32校验前置
    return FormatA("%08x_%hs", magic, tmp.c_str());
}

mstring CPacketUnique::GetTcpUniqueWithDirection(const PacketContent &packet) const {
    mstring mark = "tcp with dec";
    DWORD ipSrc = packet.m_ip_header.m_ipSource;
    DWORD ipDst = packet.m_ip_header.m_ipDestination;
    unsigned short portSrc = packet.m_tls_header.m_tcp.m_sourcePort;
    unsigned short portDst = packet.m_tls_header.m_tcp.m_destinationPort;
    DWORD addr1 = 0, addr2 = 0;
    unsigned int port1 = 0, port2 = 0;

    addr1 = ipSrc;
    port1 = portSrc;
    addr2 = ipDst;
    port2 = portDst;

    mstring tmp = FormatA("%08x_%d-%08x_%d_%hs", addr1, port1, addr2, port2, mark.c_str());
    unsigned long magic = crc32(tmp.c_str(), tmp.size(), 0xff1234aa);

    //为提高查找效率将crc32校验前置
    return FormatA("%08x_%hs", magic, tmp.c_str());
}

mstring CPacketUnique::GetIcmpUnique(const PacketContent &packet) const {
    DWORD ipSrc = packet.m_ip_header.m_ipSource;
    DWORD ipDst = packet.m_ip_header.m_ipDestination;
    DWORD addr1 = 0, addr2 = 0;
    mstring mark = "icmp";

    if (ipSrc < ipDst)
    {
        addr1 = ipSrc;
        addr2 = ipDst;
    }
    else if (ipSrc == ipDst)
    {
        addr1 = ipSrc;
        addr2 = ipDst;
    }
    else
    {
        addr1 = ipDst;
        addr2 = ipSrc;
    }

    mstring tmp = FormatA("%08x-%08x_%hs", addr1, addr1, mark.c_str());
    unsigned long magic = crc32(tmp.c_str(), tmp.size(), 0xff1234aa);

    //为提高查找效率将crc32校验前置
    return FormatA("%08x_%hs", magic, tmp.c_str());
}