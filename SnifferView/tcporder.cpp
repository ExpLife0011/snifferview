/*
 *filename:  tcporder.cpp
 *author:    lougd
 *created:   2015-7-2 17:44
 *version:   1.0.0.1
 *desc:      这个异步模型可能会导致tcp顺序错乱，这个接口是用来纠正这个错误的
 *           这个接口位于http观察者接口之前，处理过的tcp封包才能交给http观察者处理					
 *history:
*/
#include <WinSock2.h>
#include <Windows.h>
#include <map>
#include "analysis.h"

using namespace std;

struct TcpSerial
{
	UINT m_cur_num;
	UINT m_next_num;
};

//封包标识，tcp序号
map<mstring, TcpSerial> s_tcp_serials;
//封包标识，乱序的tcp封包列表
map<mstring, list<PacketContent>> s_tcp_packets;

BOOL WINAPI TcporderWatcher(IN OUT PacketContent &msg)
{
	if (msg.m_tls_type != em_tls_tcp)
	{
		return TRUE;
	}

	if (msg.m_dec_mark.size() == 0)
	{
		MessageBoxA(0, "ccc", 0, 0);
		return TRUE;
	}

	mstring mark = msg.m_dec_mark;
	TCPHeader *tcp = (TCPHeader *)msg.m_packet.c_str() + sizeof(IPHeader);
	TCPHeader vv = *tcp;
	int length = 0;
	length += sizeof(IPHeader);
	vv.n2h();
	length += vv.get_tcp_header_length();
	int user = msg.m_packet.size() - length;
	UINT vs = vv.m_sequenceNumber;
	if (s_tcp_serials.find(mark) == s_tcp_serials.end())
	{
		s_tcp_serials[mark].m_cur_num = vv.m_sequenceNumber;
		ULONGLONG data = vv.m_sequenceNumber + user;
		if (data > 0xffffffff)
		{
		}
		s_tcp_serials[mark].m_next_num = (UINT)data;
	}
	else
	{
		UINT seq = s_tcp_serials[mark].m_next_num;
		if (vs > seq)
		{
			//乱序的tcp封包
			s_tcp_packets[mark].push_back(msg);
		}
		else if(vs == seq)
		{
			//正确的tcp封包
		}
		else
		{
			//错误的封包已经发出去了，不再做处理
		}
	}
	return TRUE;
}