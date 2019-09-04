#pragma  once
#include <Windows.h>
#include "../ComLib/common.h"

#define ETHERTYPE_IP		0x0800
#define ETHERTYPE_ARP		0x0806

typedef struct _ETHeader							//14�ֽڵ���̫ͷ
{
	UCHAR	m_dhost[6];								//Ŀ��MAC��ַdestination mac address
	UCHAR	m_shost[6];								//ԴMAC��ַsource mac address
	USHORT m_type;									//�²�Э�����ͣ���IP��ETHERTYPE_IP����ARP��ETHERTYPE_ARP����
} ETHeader, *PETHeader;


#define ARPHRD_ETHER 	1
// ARPЭ��opcodes
#define	ARPOP_REQUEST	1						//ARP ����	
#define	ARPOP_REPLY		2						//ARP ��Ӧ

typedef struct _ARPHeader							//28�ֽڵ�ARPͷ
{
	USHORT m_hrd;										//Ӳ����ַ�ռ䣬��̫����ΪARPHRD_ETHER
	USHORT m_eth_type;								//��̫������ETHERTYPE_IP
	UCHAR m_maclen;									//MAC��ַ�ĳ��� 6
	UCHAR m_iplen;										//IP��ַ�ĳ��� 4
	USHORT m_opcode;								//�������� ARPOP_REQUESTΪ���� ARPOP_REPLYΪ��Ӧ
	UCHAR	m_smac[6];								//ԴMAC��ַ
	UCHAR	m_saddr[4];								//ԴIP��ַ
	UCHAR	m_dmac[6];								//Ŀ��MAC��ַ
	UCHAR	m_daddr[4];								//Ŀ��IP��ַ
} ARPHeader, *PARPHeader;


//�����Э��
#define PROTO_ICMP      1
#define PROTO_IGMP      2
#define PROTO_TCP       6
#define PROTO_UDP       17

typedef struct IPHeader								//20�ֽڵ�IPͷ
{
    UCHAR m_iphVerLen;								//�汾�ź�ͷ���� ��ռ4λ
    UCHAR m_ipTOS;									//��������
    USHORT m_ipLength;								//����ܳ��ȣ�������IP���ĳ���
    USHORT m_ipID;									//�����ʶ��Ωһ��ʶ���͵�ÿһ�����ݱ�
    USHORT m_ipFlags;								//��־
    UCHAR m_ipTTL;									//����ʱ�䣬����TTL
    UCHAR m_ipProtocol;								//Э�飬������TCP��UDP��ICMP��
    USHORT m_ipChecksum;							//У���
    ULONG m_ipSource;								//ԴIP��ַ
    ULONG m_ipDestination;							//Ŀ��IP��ַ

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


// ����TCP��־
#define   TCP_FIN   0x01
#define   TCP_SYN   0x02
#define   TCP_RST   0x04
#define   TCP_PSH   0x08
#define   TCP_ACK   0x10
#define   TCP_URG   0x20
#define   TCP_ACE   0x40
#define   TCP_CWR   0x80

typedef struct TCPHeader							//20�ֽڵ�TCPͷ
{
	USHORT m_sourcePort;							//16λԴ�˿ں�
	USHORT m_destinationPort;					//16λĿ�Ķ˿ں�
	ULONG	m_sequenceNumber;					//32λ���к�
	ULONG	m_acknowledgeNumber;			//32λȷ�Ϻ�
	UCHAR	m_tcp_offset;								//��4λ��ʾ����ƫ��
	UCHAR	m_flags;										//6λ��־λ
																//FIN - 0x01
																//SYN - 0x02
																//RST - 0x04 
																//PUSH- 0x08
																//ACK- 0x10
																//URG- 0x20

	USHORT m_windows;								//16λ���ڴ�С
	USHORT m_checksum;							//16λУ���
	USHORT m_urgentPointer;						// 16λ��������ƫ���� 

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
	USHORT			m_sourcePort;					//Դ�˿ں�		
	USHORT			m_destinationPort;				//Ŀ�Ķ˿ں�		
	USHORT			m_len;								//�������
	USHORT			m_checksum;						//У���

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