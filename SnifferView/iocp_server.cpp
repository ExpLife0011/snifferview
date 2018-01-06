#include "iocp_server.h"
#include "packets.h"
#include "analysis.h"

IocpServer::IocpServer()
{
	m_suspend = false;
	m_start = false;
	m_addr = 0;
	m_port = 0;
	m_sock = INVALID_SOCKET;
	m_event_count = 0;
	m_read_serial = 0;
	m_next_serial = 0;
	InitializeCriticalSection(&m_cs);
}

IocpServer::~IocpServer()
{
	if (m_start)
	{
		stop();
	}
	DeleteCriticalSection(&m_cs);
}

void IocpServer::suspend()
{
	lock();
	m_suspend = true;
	unlock();
}

void IocpServer::restart()
{
	lock();
	m_suspend = false;
	unlock();
}

bool IocpServer::is_start()
{
	return m_start;
}

bool IocpServer::is_sniffer()
{
	return (m_start && !m_suspend);
}

void IocpServer::lock()
{
	EnterCriticalSection(&m_cs);
}

void IocpServer::unlock()
{
	LeaveCriticalSection(&m_cs);
}

bool IocpServer::post_recv_request(PIOCP_BUFFER_OBJ buffer)
{
	DWORD bytes = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.buf = (char *)buffer->m_buffer;
	buf.len = IOCP_BUFFER_SIZE;
	bool ret = true;
	if (NO_ERROR != WSARecv(buffer->m_sock, &buf, 1, &bytes, &flags, &buffer->m_io_ol, NULL))
	{
		ret = (WSA_IO_PENDING == WSAGetLastError());
	}

	if (ret)
	{
		buffer->m_serial = m_read_serial++;
	}
	return ret;
}

bool IocpServer::post_recv_request(SOCKET sock)
{
	PIOCP_BUFFER_OBJ buffer = new IOCP_BUFFER_OBJ;
	ZeroMemory(buffer, sizeof(IOCP_BUFFER_OBJ));
	buffer->m_sock = sock;
	DWORD bytes = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.buf = (char *)buffer->m_buffer;
	buf.len = IOCP_BUFFER_SIZE;
	bool ret = true;
	if (NO_ERROR != WSARecv(buffer->m_sock, &buf, 1, &bytes, &flags, &buffer->m_io_ol, NULL))
	{
		ret = (WSA_IO_PENDING == WSAGetLastError());
	}
	if (!ret)
	{
		delete buffer;
	}
	else
	{
		buffer->m_serial = m_read_serial++;
	}
	return ret;
}

DWORD IocpServer::WorkThread(LPVOID p)
{
	IocpServer *vs = (IocpServer *)p;
	DWORD trans = 0;
	DWORD key = 0;
	LPOVERLAPPED lpol;
	PIOCP_BUFFER_OBJ buffer;
	while(TRUE)
	{
		BOOL ret = GetQueuedCompletionStatus(vs->m_iocp_handle, &trans, (LPDWORD)&key, (LPOVERLAPPED *)&lpol, WSA_INFINITE);
		if (trans == -1)
		{
			//通知工作线程退出
			break;
		}
		buffer = CONTAINING_RECORD(lpol, IOCP_BUFFER_OBJ, m_io_ol);
		if(!ret)
		{
			break;
		}
		vs->lock();
		vs->hand_io(buffer, trans, key);
		vs->unlock();
	}
	return 0;
}

bool IocpServer::hand_io(PIOCP_BUFFER_OBJ buffer, DWORD trans, DWORD key)
{
	if (trans <= 0)
	{
		MessageBoxA(0, "iocp error", 0, 0);
		return false;
	}
	//获取正确顺序的封包
	if(buffer->m_serial == m_next_serial)
	{
		if (!m_suspend)
		{
			PushPacket(buffer->m_buffer, trans);
			NoticePacket();
		}
		m_next_serial++;
		map<UINT, mstring>::iterator itu = m_packets.begin();
		while(itu != m_packets.end())
		{
			if (itu->first != m_next_serial)
			{
				break;
			}
			else
			{
				if (!m_suspend)
				{
					PushPacket(itu->second.c_str(), itu->second.size());
					NoticePacket();
				}
				itu = m_packets.erase(itu);
				m_next_serial++;
			}
		}
	}
	else
	{
		m_packets[buffer->m_serial] = mstring(buffer->m_buffer, trans);
	}
	post_recv_request(buffer);
	return true;
}

bool IocpServer::run(ULONG addr, short port)
{
	if (m_start)
	{
		return true;
	}
	SOCKET sock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN addr_in;

	addr_in.sin_family  = AF_INET;
	addr_in.sin_port    = htons(0);
	memcpy(&addr_in.sin_addr.S_un.S_addr, &addr, sizeof(addr));
	if(SOCKET_ERROR == bind(sock, (PSOCKADDR)&addr_in, sizeof(addr_in)))
	{
		return false;
	}

	DWORD res = 1;
	if(0 != ioctlsocket(sock, SIO_RCVALL, &res))		
	{
		return false;
	}
	m_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort((HANDLE)sock, m_iocp_handle, (DWORD)0, 0);
	int its = 0;
	for (its = 0 ; its < IOCP_WORKTHREAD_COUNT ; its++)
	{
		m_work_threads[its] = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
	}
	Sleep(0);
	for (its = 0 ; its < IOCP_RECV_REQUEST_COUNT ; its++)
	{
		post_recv_request(sock);
	}
	m_start = true;
	return true;
}

bool IocpServer::stop()
{
	if (m_start)
	{
		m_start = false;
		//通知所有io处理线程退出
		int itk = 0;
		for(itk = 0 ; itk < IOCP_WORKTHREAD_COUNT ; itk++)
		{
			PostQueuedCompletionStatus(m_iocp_handle, -1, 0, NULL);
		}
		if (WSA_WAIT_TIMEOUT == WaitForMultipleObjects(IOCP_WORKTHREAD_COUNT, m_work_threads, TRUE, 3000))
		{
			for (itk = 0 ; itk < IOCP_WORKTHREAD_COUNT ; itk++)
			{
				TerminateThread(m_work_threads[itk], 0);
			}
		}
		closesocket(m_sock);
		CloseHandle(m_iocp_handle);
		m_iocp_handle = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
	}
	return true;
}