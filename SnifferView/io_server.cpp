#include "io_server.h"
#include "analysis.h"

OverlappedServer::OverlappedServer()
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

OverlappedServer::~OverlappedServer()
{
	if (m_start)
	{
		stop();
	}
	DeleteCriticalSection(&m_cs);
}

void OverlappedServer::suspend()
{
	lock();
	m_suspend = true;
	unlock();
}

void OverlappedServer::restart()
{
	lock();
	m_suspend = false;
	unlock();
}

bool OverlappedServer::is_start()
{
	return m_start;
}

bool OverlappedServer::is_sniffer()
{
	return (m_start && !m_suspend);
}

void OverlappedServer::lock()
{
	EnterCriticalSection(&m_cs);
}

void OverlappedServer::unlock()
{
	LeaveCriticalSection(&m_cs);
}

bool OverlappedServer::post_recv_request(PIO_BUFFER_OBJ buffer)
{
	DWORD bytes = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.buf = (char *)buffer->m_buffer;
	buf.len = IO_BUFFER_SIZE;
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

bool OverlappedServer::post_recv_request(SOCKET sock)
{
	PIO_BUFFER_OBJ buffer = new IO_BUFFER_OBJ;
	ZeroMemory(buffer, sizeof(IO_BUFFER_OBJ));
	buffer->m_io_ol.hEvent = WSACreateEvent();
	buffer->m_sock = sock;
	DWORD bytes = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.buf = (char *)buffer->m_buffer;
	buf.len = IO_BUFFER_SIZE;
	bool ret = true;
	if (NO_ERROR != WSARecv(buffer->m_sock, &buf, 1, &bytes, &flags, &buffer->m_io_ol, NULL))
	{
		ret = (WSA_IO_PENDING == WSAGetLastError());
	}
	if (!ret)
	{
		WSACloseEvent(buffer->m_io_ol.hEvent);
		delete buffer;
	}
	else
	{
		lock();
		m_events_map[buffer->m_io_ol.hEvent] = buffer;
		m_events[m_event_count++] = buffer->m_io_ol.hEvent;
		buffer->m_serial = m_read_serial++;
		unlock();
	}
	return ret;
}

bool OverlappedServer::run(ULONG addr, short port)
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

	m_leave_event = m_events[m_event_count++] = WSACreateEvent();
	int its = 0;
	for (its = 0 ; its < IO_RECV_REQUEST_COUNT ; its++)
	{
		post_recv_request(sock);
	}

	m_sock = sock;
	m_start = true;
	for (its = 0 ; its < IO_WORKTHREAD_COUNT ; its++)
	{
		m_work_threads[its] = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
	}
	Sleep(1000);
	return true;
}

bool OverlappedServer::stop()
{
	if (m_start)
	{
		m_start = false;
		WSASetEvent(m_leave_event);
		if (WSA_WAIT_TIMEOUT == WaitForMultipleObjects(IO_WORKTHREAD_COUNT, m_work_threads, TRUE, 3000))
		{
			int itk = 0;
			for (itk = 0 ; itk < IO_WORKTHREAD_COUNT ; itk++)
			{
				TerminateThread(m_work_threads[itk], 0);
			}
		}
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		map<HANDLE, PIO_BUFFER_OBJ>::iterator itm;
		for (itm = m_events_map.begin() ; itm != m_events_map.end() ; itm++)
		{
			WSACloseEvent(itm->second->m_io_ol.hEvent);
			//delete itm->second;
		}
		m_events_map.clear();
		WSACloseEvent(m_leave_event);
		m_leave_event = NULL;
		ZeroMemory(m_events, sizeof(m_events));
		m_event_count = 0;
	}
	return true;
}

bool OverlappedServer::hand_io(PIO_BUFFER_OBJ buffer)
{
	if (!buffer)
	{
		return false;
	}
	DWORD trans = 0;
	DWORD flags = 0;
	BOOL ret = WSAGetOverlappedResult(buffer->m_sock, &buffer->m_io_ol, &trans, FALSE, &flags);
	if (!ret)
	{
		if (10040 != WSAGetLastError())
		{
			if ( INVALID_SOCKET != buffer->m_sock)
			{
				closesocket(buffer->m_sock);
				buffer->m_sock = INVALID_SOCKET;
			}
			return false;
		}
	}

	if (trans <= 0)
	{
		return false;
	}

	//获取正确顺序的封包
	if(buffer->m_serial == m_next_serial)
	{
		lock();
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
		unlock();
	}
	else
	{
		m_packets[buffer->m_serial] = mstring(buffer->m_buffer, trans);
	}

	//lock();
	//if (!m_suspend)
	//{
	//	PushPacket(buffer->m_buffer, trans);
	//	NoticePacket();
	//}
	//unlock();
	post_recv_request(buffer);
	return true;
}

DWORD OverlappedServer::WorkThread(LPVOID p)
{
	OverlappedServer *os = (OverlappedServer *)p;
	int itm = 0;
	while(os->m_start)
	{
		int idex = WSAWaitForMultipleEvents(os->m_event_count, os->m_events, FALSE, WSA_INFINITE, FALSE);
		if (WSA_WAIT_FAILED == idex)
		{
			break;
		}

		idex = idex - WSA_WAIT_EVENT_0;
		if (0 == idex)
		{
			break;
		}

		for (itm = idex ; itm < (int)os->m_event_count ; itm++)
		{
			int ret = WSAWaitForMultipleEvents(1, &os->m_events[itm], TRUE, 0, FALSE);
			if (WSA_WAIT_TIMEOUT == ret)
			{
				continue;
			}
			else
			{
			}
			//如果只投递一个recv请求的话可能会因为接受效率下降导致丢包
			//如果同时投递多个的话事件数组的顺序要进行处理，否则会导致封包顺序错乱
			WSAResetEvent(os->m_events[itm]);
			os->lock();
			HANDLE evn = os->m_events[itm];
			if (os->m_events_map.end() != os->m_events_map.find(evn))
			{
				os->hand_io(os->m_events_map[evn]);
			}
			os->unlock();
		}
	}
	return 0;
}