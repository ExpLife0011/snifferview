/*
 *filename:  tcporder.cpp
 *author:    lougd
 *created:   2015-7-2 17:44
 *version:   1.0.0.1
 *desc:      ����첽ģ�Ϳ��ܻᵼ��tcp˳����ң�����ӿ�������������������
 *           ����ӿ�λ��http�۲��߽ӿ�֮ǰ���������tcp������ܽ���http�۲��ߴ���					
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

//�����ʶ��tcp���
map<mstring, TcpSerial> s_tcp_serials;
//�����ʶ�������tcp����б�
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
			//�����tcp���
			s_tcp_packets[mark].push_back(msg);
		}
		else if(vs == seq)
		{
			//��ȷ��tcp���
		}
		else
		{
			//����ķ���Ѿ�����ȥ�ˣ�����������
		}
	}
	return TRUE;
}