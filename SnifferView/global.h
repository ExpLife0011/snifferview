#pragma once
#include <Windows.h>
#include "../ComLib/mstring.h"
#include "../ComLib/tpool.h"

using namespace std;

#ifdef  OVERLAP_SERVER
    #define IO_SERVER OverlappedServer
#else
    #define IO_SERVER IocpServer
#endif

enum WorkState
{
    em_work_launcher,
    em_work_sniffer,
    em_work_analysis,
    em_work_service,
    em_work_user
};

#define  SNIFFER_DATA_EXT    (".vex")
#define  SNIFFER_DATA_KEY    ("vexfile")
#define  SNIFFER_DATA_DEC    ("SnifferView嗅探工具的网络封包数据文件，可以通过SnifferView进行分析查看。")

#define  EVENT_USER_PROC     ("Global\\85891fbc-1aa7-40de-9789-30c991b1db22\\UserProc")

extern WorkState g_work_state;
extern mstring g_sniffer_file;
extern BOOL g_analysis_state;

extern mstring gInstallPath;
extern mstring gCfgPath;

extern HINSTANCE g_m;
extern SECURITY_ATTRIBUTES g_sa;
extern SECURITY_DESCRIPTOR g_sd;
extern ThreadPool *gThreadPool;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);