#pragma  once
#include <Windows.h>
#include "../ComLib/common.h"

#define ETHERTYPE_IP		0x0800
#define ETHERTYPE_ARP		0x0806

typedef struct _ETHeader							//14字节的以太头
{
	UCHAR	m_dhost[6];								//目的MAC地址destination mac address
	UCHAR	m_shost[6];								//源MAC地址source mac address
	USHORT m_type;									//下层协议类型，如IP（ETHERTYPE_IP）、ARP（ETHERTYPE_ARP）等
} ETHeader, *PETHeader;


#define ARPHRD_ETHER 	1
// ARP协议opcodes
#define	ARPOP_REQUEST	1						//ARP 请求	
#define	ARPOP_REPLY		2						//ARP 响应

typedef struct _ARPHeader							//28字节的ARP头
{
	USHORT m_hrd;										//硬件地址空间，以太网中为ARPHRD_ETHER
	USHORT m_eth_type;								//以太网类型ETHERTYPE_IP
	UCHAR m_maclen;									//MAC地址的长度 6
	UCHAR m_iplen;										//IP地址的长度 4
	USHORT m_opcode;								//操作代码 ARPOP_REQUEST为请求 ARPOP_REPLY为响应
	UCHAR	m_smac[6];								//源MAC地址
	UCHAR	m_saddr[4];								//源IP地址
	UCHAR	m_dmac[6];								//目的MAC地址
	UCHAR	m_daddr[4];								//目的IP地址
} ARPHeader, *PARPHeader;


//网络层协议
#define PROTO_ICMP      1
#define PROTO_IGMP      2
#define PROTO_TCP       6
#define PROTO_UDP       17

typedef struct IPHeader								//20字节的IP头
{
    UCHAR m_iphVerLen;								//版本号和头长度 各占4位
    UCHAR m_ipTOS;									//服务类型
    USHORT m_ipLength;								//封包总长度，即整个IP报的长度
    USHORT m_ipID;									//封包标识，惟一标识发送的每一个数据报
    USHORT m_ipFlags;								//标志
    UCHAR m_ipTTL;									//生存时间，就是TTL
    UCHAR m_ipProtocol;								//协议，可能是TCP、UDP、ICMP等
    USHORT m_ipChecksum;							//校验和
    ULONG m_ipSource;								//源IP地址
    ULONG m_ipDestination;							//目标IP地址

	void n2h()
	{
		m_ipLength = n2h_16(m_ipLength);
		m_ipID = n2h_16(m_ipID);
		m_ipFlags = n2h_16(m_ipFlags);
		m_ipChecksum = n2h_16(m_ipChecksum);
		m_ipSource = n2h_32(m_ipSource);
		m_ipDestination = n2h_32(m_ipDestination);
	}
} IPHeader, *PIPHeader; 


// 定义TCP标志
#define   TCP_FIN   0x01
#define   TCP_SYN   0x02
#define   TCP_RST   0x04
#define   TCP_PSH   0x08
#define   TCP_ACK   0x10
#define   TCP_URG   0x20
#define   TCP_ACE   0x40
#define   TCP_CWR   0x80

typedef struct TCPHeader							//20字节的TCP头
{
	USHORT m_sourcePort;							//16位源端口号
	USHORT m_destinationPort;					//16位目的端口号
	ULONG	m_sequenceNumber;					//32位序列号
	ULONG	m_acknowledgeNumber;			//32位确认号
	UCHAR	m_tcp_offset;								//高4位表示数据偏移
	UCHAR	m_flags;										//6位标志位
																//FIN - 0x01
																//SYN - 0x02
																//RST - 0x04 
																//PUSH- 0x08
																//ACK- 0x10
																//URG- 0x20

	USHORT m_windows;								//16位窗口大小
	USHORT m_checksum;							//16位校验和
	USHORT m_urgentPointer;						// 16位紧急数据偏移量 

	size_t get_tcp_header_length()
	{
		return (((m_tcp_offset & 0xf0) >> 4) * 4);
	}

	bool is_flag_fin()
	{
		return (m_flags & 0x01) != 0;
	}

	bool is_flag_syn()
	{
		return (m_flags & 0x02) != 0;
	}

	bool is_flag_rst()
	{
		return (m_flags & 0x04) != 0;
	}

	bool is_flag_push()
	{
		return (m_flags & 0x08) != 0;
	}

	bool is_flag_ack()
	{
		return (m_flags & 0x10) != 0;
	}

	bool is_flag_urg()
	{
		return (m_flags & 0x20) != 0;
	}

	void n2h()
	{
		m_sourcePort = n2h_16(m_sourcePort);
		m_destinationPort = n2h_16(m_destinationPort);
		m_sequenceNumber = n2h_32(m_sequenceNumber);
		m_acknowledgeNumber = n2h_32(m_acknowledgeNumber);
		m_windows = n2h_16(m_windows);
		m_checksum = n2h_16(m_checksum);
		m_urgentPointer = n2h_16(m_urgentPointer);
	}
} TCPHeader, *PTCPHeader;

typedef struct UDPHeader
{
	USHORT			m_sourcePort;					//源端口号		
	USHORT			m_destinationPort;				//目的端口号		
	USHORT			m_len;								//封包长度
	USHORT			m_checksum;						//校验和

	void n2h()
	{
		m_sourcePort = n2h_16(m_sourcePort);
		m_destinationPort = n2h_16(m_destinationPort);
		m_len = n2h_16(m_len);
		m_checksum = n2h_16(m_checksum);
	}
} UDPHeader, *PUDPHeader;

typedef struct ICMPHeader
{
	CHAR m_type;
	CHAR m_code;
	USHORT m_checksum;
	USHORT m_id;
	USHORT m_sequence;
	ULONG m_timestAmp;

	void n2h()
	{
		m_checksum = n2h_16(m_checksum);
		m_id = n2h_16(m_id);
		m_sequence = n2h_16(m_sequence);
		m_timestAmp = n2h_32(m_timestAmp);
	}
}ICMPHeader, *PICMPHeader;