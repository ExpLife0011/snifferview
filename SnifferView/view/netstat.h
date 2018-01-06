#pragma  once
#include <Windows.h>
#include <IPHlpApi.h>
#include <mstring.h>

#define  MSG_EXEC_CMD					(WM_USER + 5008)

enum ProtocolType
{
	em_netstat_unknow,
	em_netstat_tcp,
	em_netstat_udp
};

enum em_netstat_filter
{
	em_filter_path = 0,
	em_filter_pid = 1,
	em_filter_port = 2
};

//执行命令
struct NetstatCmd
{
	em_netstat_filter m_pos;
	mstring m_cmd;
};

struct ProcessInfo
{
	ProcessInfo()
	{
		m_pid = -1;
		int m_image_idex = -1;
		m_ico = NULL;
	}
	DWORD m_pid;					//进程的Pid
	mstring m_path;					//进程全路径
	mstring m_name;				//进程名
	int m_image_idex;				//进程ico索引
	HICON m_ico;					//进程ico句柄
};

typedef struct NetstatInfo
{
	ProtocolType m_type;											//网络协议类型
	//int m_idex;															//在ListView中的序号
	union Netstate
	{
		MIB_TCPROW_OWNER_PID m_tcp_state;			//tcp state
		MIB_UDPROW_OWNER_PID m_udp_state;		//udp state
	};
	Netstate m_state;
	int m_Pid;															//进程Pid
	ProcessInfo m_process;										//连接对应的进程信息
	NetstatInfo()
	{
		m_type = em_netstat_unknow;
		//m_idex = -1;
		m_Pid = -1;
	}

	bool operator > (const NetstatInfo &dst) const
	{
		if (dst.m_type != m_type)
		{
			return false;
		}

		mstring ms;
		mstring mv;
		if (dst.m_type == em_netstat_unknow)
		{
			return false;
		}
		else if (dst.m_type == em_netstat_tcp)
		{
			ms.format("%08x%08x%08x%08x%08x%08x", m_state.m_tcp_state.dwLocalAddr, m_state.m_tcp_state.dwLocalPort, m_state.m_tcp_state.dwOwningPid, m_state.m_tcp_state.dwRemoteAddr, m_state.m_tcp_state.dwRemotePort, m_state.m_tcp_state.dwState);
			mv.format("%08x%08x%08x%08x%08x%08x", dst.m_state.m_tcp_state.dwLocalAddr, dst.m_state.m_tcp_state.dwLocalPort, dst.m_state.m_tcp_state.dwOwningPid, dst.m_state.m_tcp_state.dwRemoteAddr, dst.m_state.m_tcp_state.dwRemotePort, dst.m_state.m_tcp_state.dwState);
		}
		else
		{
			ms.format("%08x%08x%08x", m_state.m_udp_state.dwLocalAddr, m_state.m_udp_state.dwLocalPort, m_state.m_udp_state.dwOwningPid);
			mv.format("%08x%08x%08x", dst.m_state.m_udp_state.dwLocalAddr, dst.m_state.m_udp_state.dwLocalPort, dst.m_state.m_udp_state.dwOwningPid);
		}
		return ms > mv;
	}

	bool operator < (const NetstatInfo &dst) const
	{
		if (dst.m_type != m_type)
		{
			return false;
		}

		mstring ms;
		mstring mv;
		if (dst.m_type == em_netstat_unknow)
		{
			return false;
		}
		else if (dst.m_type == em_netstat_tcp)
		{
			ms.format("%08x%08x%08x%08x%08x%08x", m_state.m_tcp_state.dwLocalAddr, m_state.m_tcp_state.dwLocalPort, m_state.m_tcp_state.dwOwningPid, m_state.m_tcp_state.dwRemoteAddr, m_state.m_tcp_state.dwRemotePort, m_state.m_tcp_state.dwState);
			mv.format("%08x%08x%08x%08x%08x%08x", dst.m_state.m_tcp_state.dwLocalAddr, dst.m_state.m_tcp_state.dwLocalPort, dst.m_state.m_tcp_state.dwOwningPid, dst.m_state.m_tcp_state.dwRemoteAddr, dst.m_state.m_tcp_state.dwRemotePort, dst.m_state.m_tcp_state.dwState);
		}
		else
		{
			ms.format("%08x%08x%08x", m_state.m_udp_state.dwLocalAddr, m_state.m_udp_state.dwLocalPort, m_state.m_udp_state.dwOwningPid);
			mv.format("%08x%08x%08x", dst.m_state.m_udp_state.dwLocalAddr, dst.m_state.m_udp_state.dwLocalPort, dst.m_state.m_udp_state.dwOwningPid);
		}
		return ms < mv;
	}

	bool operator == (const NetstatInfo &dst) const
	{
		if (dst.m_type != m_type)
		{
			return false;
		}

		mstring ms;
		mstring mv;
		if (dst.m_type == em_netstat_unknow)
		{
			return true;
		}
		else if (dst.m_type == em_netstat_tcp)
		{
			ms.format("%08x%08x%08x%08x%08x%08x", m_state.m_tcp_state.dwLocalAddr, m_state.m_tcp_state.dwLocalPort, m_state.m_tcp_state.dwOwningPid, m_state.m_tcp_state.dwRemoteAddr, m_state.m_tcp_state.dwRemotePort, m_state.m_tcp_state.dwState);
			mv.format("%08x%08x%08x%08x%08x%08x", dst.m_state.m_tcp_state.dwLocalAddr, dst.m_state.m_tcp_state.dwLocalPort, dst.m_state.m_tcp_state.dwOwningPid, dst.m_state.m_tcp_state.dwRemoteAddr, dst.m_state.m_tcp_state.dwRemotePort, dst.m_state.m_tcp_state.dwState);
		}
		else
		{
			ms.format("%08x%08x%08x", m_state.m_udp_state.dwLocalAddr, m_state.m_udp_state.dwLocalPort, m_state.m_udp_state.dwOwningPid);
			mv.format("%08x%08x%08x", dst.m_state.m_udp_state.dwLocalAddr, dst.m_state.m_udp_state.dwLocalPort, dst.m_state.m_udp_state.dwOwningPid);
		}
		return ms == mv;
	}
}NetstatInfo, *PNetstatInfo;

//获取tcp网络连接状态列表
BOOL GetTcpConnectTable(OUT mstring &buffer);

//获取udp网络连接状态列表
BOOL GetUdpConnectTable(OUT mstring &buffer);

VOID StartNetstat();

VOID StopNetstat();

HWND RunNetstatView(HWND parent, LPVOID param);