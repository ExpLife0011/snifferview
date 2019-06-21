#pragma once
#include <Windows.h>
#include <mstring.h>

using namespace std;

#ifdef  OVERLAP_SERVER
	#define IO_SERVER OverlappedServer
#else
	#define IO_SERVER IocpServer
#endif

enum UserState
{
    em_user,
    em_admin,
    em_system
};

enum WorkState
{
	em_sniffer,
	em_analysis,
    em_service
};

#define  SNIFFER_DATA_EXT		(".vex")
#define  SNIFFER_DATA_KEY		("vexfile")
#define  SNIFFER_DATA_DEC		("SnifferView嗅探工具的网络封包数据文件，可以通过SnifferView进行分析查看。")

extern WorkState g_work_state;
extern mstring g_sniffer_file;
extern BOOL g_analysis_state;

extern mstring gInstallPath;
extern mstring gCfgPath;

extern HINSTANCE g_m;
extern SECURITY_ATTRIBUTES g_sa;
extern SECURITY_DESCRIPTOR g_sd;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);