/*
 *	filename:  iocp_server.cpp
 *	author:	    lougd
 *	created:    2015-7-3 13:32
 *	version:    1.0.0.1
 *	desc:
 *  history:
*/
#pragma  once
#include <WinSock2.h>
#include <Windows.h>
#include <mstcpip.h>
#include <map>
#include <vector>
#include <list>
#include "../ComLib/common.h"

#pragma  comment(lib, "ws2_32.lib")
using namespace std;

#define  IOCP_RECV_REQUEST_COUNT	16
#define  IOCP_WORKTHREAD_COUNT		4

#define OP_ACCEPT	1
#define OP_READ		2
#define OP_WRITE		3

#define  IOCP_BUFFER_SIZE 0xffff

typedef struct IOCP_BUFFER_OBJ
{	
	OVERLAPPED m_io_ol;								//重叠结构
	CHAR m_buffer[IOCP_BUFFER_SIZE];			//send/recv/AcceptEx所使用的缓冲区
	SOCKET m_sock;									//此I/O所属的监听套节字对象
	UINT m_serial;										//本次读请求的序号，同时投递多个读请求的话可能会导致封包顺序错乱，这个序号就是纠正这个错误的
}IOCP_BUFFER_OBJ, *PIOCP_BUFFER_OBJ;

class IocpServer
{
public:
	IocpServer();
	virtual ~IocpServer();

public:
	bool run(DWORD addr, short port);
	bool stop();
	bool is_start();
	bool is_sniffer();

	void suspend();
	void restart();
protected:
	bool post_recv_request(PIOCP_BUFFER_OBJ buffer);
	bool post_recv_request(SOCKET sock);
	void lock();
	void unlock();

protected:
	bool hand_io(PIOCP_BUFFER_OBJ buffer, DWORD trans, DWORD key);

protected:
	static DWORD WINAPI WorkThread(LPVOID p);

protected:
	//纠正封包获取顺序错误的数据
	map<UINT, mstring> m_packets; //用于缓存序号错误的封包数据
	UINT m_read_serial;	//标记下次投递读请求需要设置的序列号
	UINT m_next_serial;	//标记当前要获取的序列号

	CRITICAL_SECTION m_cs;
	size_t m_event_count;
	bool m_start;
	bool m_suspend;
	DWORD m_addr;
	short m_port;
	HANDLE m_iocp_handle;
	HANDLE m_work_threads[IOCP_WORKTHREAD_COUNT];
	SOCKET m_sock;
};