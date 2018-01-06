/*
 *filename: connect.cpp
 *author:   lougd
 *created:  2015-6-29 11:00
 *version:  1.0.0.1
 *desc:     链接初始化接口，区分不同的链接
 *history:
*/
#include <WinSock2.h>
#include <Windows.h>
#include <winnt.h>
#include <mstring.h>
#include <map>
#include <set>
#include "analysis.h"

using namespace std;

static HANDLE s_connect_lock = CreateMutexA(NULL, FALSE, NULL);
#define  LOCK_CONNECT		WaitForSingleObject(s_connect_lock, INFINITE);
#define  UNLOCK_CONNECT	ReleaseMutex(s_connect_lock)

static map<ConnectMark, mstring> s_connect_marks;

VOID WINAPI ClearConnect()
{
	LOCK_CONNECT;
	s_connect_marks.clear();
	UNLOCK_CONNECT;
}

BOOL WINAPI GetTcpMarkWithDirection(IN const mstring &packet, OUT ConnectMark &vm)
{
	if (packet.size() < sizeof(IPHeader))
	{
		MessageBoxA(0, "13", 0, 0);
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)packet.c_str();
	if (ip->m_ipProtocol != PROTO_TCP)
	{
		MessageBoxA(0, "14", 0, 0);
		return FALSE;
	}
	TCPHeader *tcp = (TCPHeader *)(packet.c_str() + sizeof(IPHeader));
	vm.m_addr1 = ip->m_ipSource;
	vm.m_port1 = tcp->m_sourcePort;
	vm.m_addr2 = ip->m_ipDestination;
	vm.m_port2 = tcp->m_destinationPort;
	vm.m_mark = "tcp with dec";
	return TRUE;
}

BOOL WINAPI GetTcpMark(IN const mstring &packet, OUT ConnectMark &vm)
{
	if (packet.size() < sizeof(IPHeader))
	{
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)packet.c_str();
	if (ip->m_ipProtocol != PROTO_TCP)
	{
		return FALSE;
	}
	TCPHeader *tcp = (TCPHeader *)(packet.c_str() + sizeof(IPHeader));
	if (ip->m_ipSource < ip->m_ipDestination)
	{
		vm.m_addr1 = ip->m_ipSource;
		vm.m_port1 = tcp->m_sourcePort;
		vm.m_addr2 = ip->m_ipDestination;
		vm.m_port2 = tcp->m_destinationPort;
	}
	else if (ip->m_ipSource == ip->m_ipDestination)
	{
		if (tcp->m_sourcePort < tcp->m_destinationPort)
		{
			vm.m_addr1 = ip->m_ipSource;
			vm.m_port1 = tcp->m_sourcePort;
			vm.m_addr2 = ip->m_ipDestination;
			vm.m_port2 = tcp->m_destinationPort;
		}
		else
		{
			vm.m_addr1 = ip->m_ipDestination;
			vm.m_port1 = tcp->m_destinationPort;
			vm.m_addr2 = ip->m_ipSource;
			vm.m_port2 = tcp->m_sourcePort;
		}
	}
	else
	{
		vm.m_addr1 = ip->m_ipDestination;
		vm.m_port1 = tcp->m_destinationPort;
		vm.m_addr2 = ip->m_ipSource;
		vm.m_port2 = tcp->m_sourcePort;
	}
	vm.m_mark = "tcp";
	return TRUE;
}

BOOL WINAPI GetUdpMark(IN const mstring &packet, OUT ConnectMark &vm)
{
	if (packet.size() < sizeof(IPHeader))
	{
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)packet.c_str();
	if (ip->m_ipProtocol != PROTO_UDP)
	{
		return FALSE;
	}
	UDPHeader *udp = (UDPHeader *)(packet.c_str() + sizeof(IPHeader));
	if (ip->m_ipSource < ip->m_ipDestination)
	{
		vm.m_addr1 = ip->m_ipSource;
		vm.m_port1 = udp->m_sourcePort;
		vm.m_addr2 = ip->m_ipDestination;
		vm.m_port2 = udp->m_destinationPort;
	}
	else if (ip->m_ipSource == ip->m_ipDestination)
	{
		if (udp->m_sourcePort < udp->m_destinationPort)
		{
			vm.m_addr1 = ip->m_ipSource;
			vm.m_port1 = udp->m_sourcePort;
			vm.m_addr2 = ip->m_ipDestination;
			vm.m_port2 = udp->m_destinationPort;
		}
		else
		{
			vm.m_addr1 = ip->m_ipDestination;
			vm.m_port1 = udp->m_destinationPort;
			vm.m_addr2 = ip->m_ipSource;
			vm.m_port2 = udp->m_sourcePort;
		}
	}
	else
	{
		vm.m_addr1 = ip->m_ipDestination;
		vm.m_port1 = udp->m_destinationPort;
		vm.m_addr2 = ip->m_ipSource;
		vm.m_port2 = udp->m_sourcePort;
	}
	vm.m_mark = "udp";
	return TRUE;
}

BOOL WINAPI GetIcmpMark(IN const mstring &packet, OUT ConnectMark &vm)
{
	if (packet.size() < sizeof(IPHeader))
	{
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)packet.c_str();
	if (ip->m_ipProtocol != PROTO_ICMP)
	{
		return FALSE;
	}
	ICMPHeader *icmp = (ICMPHeader *)(packet.c_str() + sizeof(IPHeader));
	if (ip->m_ipSource < ip->m_ipDestination)
	{
		vm.m_addr1 = ip->m_ipSource;
		vm.m_addr2 = ip->m_ipDestination;
	}
	else if (ip->m_ipSource == ip->m_ipDestination)
	{
		vm.m_addr1 = ip->m_ipSource;
		vm.m_addr2 = ip->m_ipDestination;
	}
	else
	{
		vm.m_addr1 = ip->m_ipDestination;
		vm.m_addr2 = ip->m_ipSource;
	}
	vm.m_mark = "icmp";
	return TRUE;
}

//链接初始化接口，每个链接都要首先经过这个接口来进行初始化
BOOL WINAPI ConnectInit(IN OUT PacketContent &msg)
{
	if (msg.m_packet.size() < sizeof(IPHeader))
	{
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)msg.m_packet.c_str();
	int length = sizeof(IPHeader);
	ConnectMark vm;
	ConnectMark mm;
	mstring str;
	bool create = false;
	bool flag = false;
	if (ip->m_ipProtocol == PROTO_TCP)
	{
		flag = true;
		if (msg.m_packet.size() < length + sizeof(TCPHeader))
		{
			return FALSE;
		}
		GetTcpMark(msg.m_packet, vm);
		GetTcpMarkWithDirection(msg.m_packet, mm);
		TCPHeader *tcp = (TCPHeader *)(msg.m_packet.c_str() + sizeof(IPHeader));
		if (tcp->is_flag_syn() && !(tcp->is_flag_ack()))
		{
			create = true;
		}
	}
	else if (ip->m_ipProtocol == PROTO_UDP)
	{
		if (msg.m_packet.size() < length + sizeof(UDPHeader))
		{
			return FALSE;
		}
		GetUdpMark(msg.m_packet, vm);
	}
	else if (ip->m_ipProtocol == PROTO_ICMP)
	{
		if (msg.m_packet.size() < length + sizeof(ICMPHeader))
		{
			return FALSE;
		}
		GetIcmpMark(msg.m_packet, vm);
	}
	else
	{
		return FALSE;
	}

	LOCK_CONNECT;
	if (create || s_connect_marks.end() == s_connect_marks.find(vm))
	{
		//防止产生相同的随机数
		static LONG volatile s_rand = 0;
		InterlockedExchange(&s_rand, s_rand + 1);
		if (s_rand >= 0xfffffff)
		{
			InterlockedExchange(&s_rand, 0);
		}
		srand(GetTickCount() + s_rand);
		mstring vc;
		vc.format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
		s_connect_marks[vm] = vc;

		if (flag)
		{
			ConnectMark msa;
			ConnectMark msb;
			msa = mm;
			msb.m_addr1 = mm.m_addr2;
			msb.m_port1 = mm.m_port2;
			msb.m_addr2 = mm.m_addr1;
			msb.m_port2 = mm.m_port1;
			msb.m_mark = mm.m_mark;
			if (msa == msb)
			{
				MessageBoxA(0, "10", 0, 0);
			}
			vc.format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
			s_connect_marks[msa] = vc;
			vc.format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
			s_connect_marks[msb] = vc;
		}
	}

	msg.m_packet_init = true;
	msg.m_packet_mark = s_connect_marks[vm];
	if (flag)
	{
		msg.m_dec_mark = s_connect_marks[mm];
	}
	UNLOCK_CONNECT;
	return TRUE;
}