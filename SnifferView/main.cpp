#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <map>
#include <list>
#include "../ComLib/mstring.h"
#include "../ComLib/common.h"
#include "../ComLib/firewallctl.h"
#include "../ComLib/servhlpr.h"
#include "packets.h"
#include "analysis.h"
#include "view/view.h"
#include "filter.h"
#include "view/netview.h"
#include "resource.h"
#include "global.h"
#include "config.h"
#include "servers.h"
#include "dump.h"
#include "iocp_server.h"
#include "view/netstat.h"
#include "sfvserv.h"
#include "../ComLib/StrUtil.h"
#include "../ComLib/tpool.h"
#include "UserProc/UserTask.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma  comment(lib, "comctl32.lib")

using namespace std;

WorkState g_work_state = em_work_launcher;
mstring g_sniffer_file;
BOOL g_analysis_state = FALSE;
mstring gInstallPath;
mstring gCfgPath;
//thread pool
ThreadPool *gThreadPool = NULL;

HINSTANCE g_m;
SECURITY_ATTRIBUTES g_sa;
SECURITY_DESCRIPTOR g_sd;
static DWORD gsParentPid = 0;

//命令行参数解析 /s或者空 嗅探模式， /f数据文件分析模式
static BOOL _AnalysisCmd()
{
    const char *cmd = GetCommandLineA();
    ustring um = AtoW(cmd);
    int count = 0;
    BOOL state = FALSE;
    do 
    {
        LPWSTR* args = CommandLineToArgvW(um.c_str(), &count);
        if (count < 1)
        {
            break;
        }

        if (1 == count)
        {
            g_work_state = em_work_launcher;
            g_config_path = REG_SNIFFER_CONFIG_PATH;
            state = TRUE;
        }
        else
        {
            mstring vm = WtoA(args[1]);
            vm.makelower();
            if (vm == "/s" || vm == "-s")
            {
                g_work_state = em_work_sniffer;
                g_config_path = REG_SNIFFER_CONFIG_PATH;
                state = TRUE;
            }
            else if (vm == "/f" || vm == "-f")
            {
                g_work_state = em_work_analysis;
                if (count > 2)
                {
                    g_sniffer_file = WtoA(args[2]);
                }
                state = TRUE;
                g_config_path = REG_ANALYSIS_CONFIG_PATH;
            }
            else if (vm == "/sv" || vm == "\\sv" || vm == "-sv")
            {
                g_work_state = em_work_sniffer;
                g_config_path = REG_SNIFFER_CONFIG_PATH;
                state = TRUE;
            }
            else if (vm == "-service")
            {
                g_work_state = em_work_service;
                state = TRUE;
            }
            else if (vm == "-user")
            {
                g_work_state = em_work_user;
                gsParentPid = _wtoi(args[2]);
                state = TRUE;
            }
        }
    } while (FALSE);
    return state;
}

static BOOL _InstallSnifferServ()
{
    ustring procPath = AtoW(gInstallPath);
    procPath.path_append(L"SnifferView.exe");

    BOOL bServ = TRUE;
    BOOL bStat = FALSE;
    HANDLE hNotify = NULL;
    do 
    {

        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(procPath.c_str()))
        {
            break;
        }

        hNotify = OpenEventW(EVENT_MODIFY_STATE, FALSE, SFV_NOTIFY_NAME);

        if (hNotify)
        {
            dp(L"服务进程已经启动");
            bStat = TRUE;
            break;
        }

        bServ = (InstallLocalService(procPath.c_str(), SFV_SERVICE_NAME, SFV_SERVICE_DISPLAY_NAME,SFV_SERVICE_DESCRIPTION) && StartLocalService(SFV_SERVICE_NAME));
        if (bServ)
        {
            hNotify = OpenEventW(EVENT_MODIFY_STATE, FALSE, SFV_NOTIFY_NAME);
            DWORD dwTimeCount = GetTickCount();
            while (GetTickCount() - dwTimeCount < 5000)
            {
                if (hNotify = OpenEventW(EVENT_MODIFY_STATE, FALSE, SFV_NOTIFY_NAME))
                {
                    bStat = TRUE;
                    break;
                }
                Sleep(10);
            }
        }
        else
        {
            dp(L"启动SfvServ服务失败:%d", GetLastError());
        }
    } while (FALSE);
   
    if (hNotify)
    {
        CloseHandle(hNotify);
    }
    return bStat;
}

static void _InstallModule() {
    mstring dllPath;
    dllPath = gInstallPath;

    dllPath.path_append("SyntaxView.dll");
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(dllPath.c_str()))
    {
        ReleaseRes(dllPath.c_str(), IDR_DLL_SYNTAX, "DLL");
    }

    dllPath.path_append("..\\Dumper.dll");
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(dllPath.c_str()))
    {
        ReleaseRes(dllPath.c_str(), IDR_DLL_DUMPER, "DLL");
    }

    mstring sniffPath = dllPath.path_append("..\\SnifferView.exe");
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(sniffPath.c_str()))
    {
        char selfPath[512];
        GetModuleFileNameA(NULL, selfPath, sizeof(selfPath));
        CopyFileA(selfPath, sniffPath.c_str(), TRUE);
    }
}

static void _InitSniffParam() {
#ifdef _DEBUG
    char buff[512];
    GetModuleFileNameA(NULL, buff, sizeof(buff));
    PathAppendA(buff, "..");
    gInstallPath = buff;
    PathAppendA(buff, "cache");
    gCfgPath = buff;
#else
    char installDir[512];
    GetWindowsDirectoryA(installDir, sizeof(installDir));
    PathAppendA(installDir, "SnifferView");

    char selfPath[512];
    GetModuleFileNameA(NULL, selfPath, RTL_NUMBER_OF(selfPath));

    mstring verStr;
    GetFileVersion(selfPath, verStr);
    PathAppendA(installDir, verStr.c_str());

    gInstallPath = installDir;
    PathAppendA(installDir, "cache");
    gCfgPath = installDir;
#endif
    SHCreateDirectoryExA(NULL, gCfgPath.c_str(), NULL);
    return;
}

//初始化异常处理模块 2019/09/05
static void _InitDumper() {
    typedef bool (__stdcall *pfnDumperInit)(const wchar_t *);

    mstring dllPath = gInstallPath;
    dllPath.path_append("Dumper.dll");
    pfnDumperInit pfn = (pfnDumperInit)GetProcAddress(LoadLibraryA(dllPath.c_str()), "DumperInit");

    ustring wstr = AtoW(gInstallPath);
    wstr.path_append(L"minidump");
    pfn(wstr.c_str());
}

int WINAPI WinMain(HINSTANCE m, HINSTANCE p, LPSTR cmd, int show)
{
    dp(L"SnifferView启动参数：%ls", GetCommandLineW());

    //Init Param
    _InitSniffParam();
    //安装组件
    _InstallModule();
    //初始化异常处理模块
    _InitDumper();

    g_m = m;
    if (!_AnalysisCmd())
    {
        dp(L"SnifferView参数错误");
        return 0;
    }

    InitEveryMutexACL(g_sa, g_sd);
    InitFilterEngine();
    InitSnfferViewConfig();
    gThreadPool = new ThreadPool(1, 4);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    do
    {
        if (em_work_launcher == g_work_state)
        {
            HWND hwd = FindWindowA(NULL, SNIFFER_STATE_NAME);
            if (!hwd)
            {
                hwd = FindWindowA(NULL, SNIFFER_SUSPEND_NAME);
            }
            if (hwd)
            {
                SendMessageA(hwd, MSG_ACTIVE_WINDOW, 0, 0);
                break;
            }

            if (!IsAdminUser())
            {
                MessageBoxA(0, "检测到SnifferView没有管理员权限，可能无法正常的嗅探网络数据。", "没有管理员权限", MB_OK | MB_ICONWARNING);
            }
            else
            {
                #ifndef _DEBUG
                WCHAR wszSelf[MAX_PATH] = {0};
                GetModuleFileNameW(NULL, wszSelf, MAX_PATH);
                dp(L"test1");
                if (_InstallSnifferServ())
                {
                    DWORD dwSession = 1;
                    ProcessIdToSessionId(GetCurrentProcessId(), &dwSession);
                    RunInUser(wszSelf, L"-sv", dwSession);
                    break;
                } else {
                    dp(L"test2");
                }

                if (wszSelf[0])
                {
                    WindowsFirewallAddAppW(wszSelf, PathFindFileNameW(wszSelf));
                }
                #endif
            }
            g_work_state = em_work_sniffer;
            ShowSnifferView();
        } else if (em_work_sniffer == g_work_state)
        {
            ShowSnifferView();
        }
        else if (em_work_analysis == g_work_state)
        {
            ShowSnifferView();
        }
        else if (em_work_service == g_work_state)
        {
            RunSinfferServ();
        }
        else if (em_work_user == g_work_state)
        {
            CreateEventA(NULL, FALSE, FALSE, EVENT_USER_PROC);
            CUserTaskMgr::GetInst()->StartService(gsParentPid);
        }
    } while ( FALSE);
    WSACleanup();
    return 0;
}