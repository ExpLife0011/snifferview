#pragma  once
#include <WinSock2.h>
#include <Windows.h>
#include <mstcpip.h>
#include <map>
#include <vector>
#include <list>
#include <common.h>

#pragma  comment(lib, "ws2_32.lib")
using namespace std;

#define  IO_RECV_REQUEST_COUNT	16
#define  IO_WORKTHREAD_COUNT		1

#define OP_ACCEPT	1
#define OP_READ		2
#define OP_WRITE		3

#define  IO_BUFFER_SIZE 0xffff

typedef struct IO_BUFFER_OBJ
{	
	OVERLAPPED m_io_ol;								//�ص��ṹ
	CHAR m_buffer[IO_BUFFER_SIZE];			//send/recv/AcceptEx��ʹ�õĻ�����
	SOCKET m_sock;									//��I/O�����ļ����׽��ֶ���
	UINT m_serial;										//���ζ��������ţ�ͬʱͶ�ݶ��������Ļ����ܻᵼ�·��˳����ң������ž��Ǿ�����������
}IO_BUFFER_OBJ, *PIO_BUFFER_OBJ;

class OverlappedServer
{
public:
	OverlappedServer();
	virtual ~OverlappedServer();

public:
	bool run(DWORD addr, short port);
	bool stop();
	bool is_start();
	bool is_sniffer();

	void suspend();
	void restart();
protected:
	bool post_recv_request(PIO_BUFFER_OBJ buffer);
	bool post_recv_request(SOCKET sock);
	void lock();
	void unlock();

protected:
	bool hand_io(PIO_BUFFER_OBJ buffer);

protected:
	static DWORD WINAPI WorkThread(LPVOID p);

protected:
	//���������ȡ˳����������
	map<UINT, mstring> m_packets; //���ڻ�����Ŵ���ķ������
	UINT m_read_serial;	//����´�Ͷ�ݶ�������Ҫ���õ����к�
	UINT m_next_serial;	//��ǵ�ǰҪ��ȡ�����к�

	CRITICAL_SECTION m_cs;
	map<HANDLE, PIO_BUFFER_OBJ> m_events_map;
	HANDLE m_events[WSA_MAXIMUM_WAIT_EVENTS];
	size_t m_event_count;
	bool m_start;
	bool m_suspend;
	DWORD m_addr;
	short m_port;
	HANDLE m_work_threads[IO_WORKTHREAD_COUNT];
	HANDLE m_leave_event;
	SOCKET m_sock;
};