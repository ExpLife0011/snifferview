#pragma  once
#include <WinSock2.h>
#include <Windows.h>
#include "analysis.h"

//{36D13FD1-30FD-4f2a-B0D6-B3F4A8D11669}
#define  DUMP_FLAG_TIME				(1 << 0)
#define  DUMP_FLAG_PACKET			(1 << 1)
#define  DUMP_FILE_VER				10005

struct DumpFileVersion
{
	char	m_check[128];		//校验
	DWORD m_version;		//版本号
	DWORD m_file_flag;		//标识

	DumpFileVersion()
	{
		lstrcpyA(m_check, "{36D13FD1-30FD-4f2a-B0D6-B3F4A8D11669}");
		m_version = DUMP_FILE_VER;
		m_file_flag = DUMP_FLAG_TIME;
		m_file_flag |= DUMP_FLAG_PACKET;
	}
};

BOOL WINAPI DumpPacketsToFile(IN const char *path, IN vector<PPacketContent> &packets);

BOOL WINAPI DumpPacketsFromFile(IN const char *path, OUT vector<PPacketContent> &packets);