#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <map>
#include <list>
#include <mstring.h>
#include <common.h>
#include <firewallctl.h>
#include <servhlpr.h>
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
#include "StrUtil.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma  comment(lib, "comctl32.lib")

using namespace std;

WorkState g_work_state = em_sniffer;
UserState g_eUserState = em_user;
mstring g_sniffer_file;
BOOL g_analysis_state = FALSE;
mstring gInstallPath;
mstring gCfgPath;

HINSTANCE g_m;
SECURITY_ATTRIBUTES g_sa;
SECURITY_DESCRIPTOR g_sd;

//命令行参数解析 /s或者空 嗅探模式， /f数据文件分析模式
static BOOL _AnalysisCmd()
{
    const char *cmd = GetCommandLineA();
    ustring um = AtoW(cmd);
    int count = 0;
    BOOL state = FALSE;
    do 
    {
        if (IsAdminUser())
        {
            g_eUserState = em_admin;
        }
        else
        {
            g_eUserState = em_user;
        }

        LPWSTR* args = CommandLineToArgvW(um.c_str(), &count);
        if (count < 1)
        {
            break;
        }

        if (1 == count)
        {
            g_work_state = em_sniffer;
            g_config_path = REG_SNIFFER_CONFIG_PATH;
            state = TRUE;
        }
        else
        {
            mstring vm = WtoA(args[1]);
            vm.makelower();
            if (vm == "/s" || vm == "-s")
            {
                g_work_state = em_sniffer;
                g_config_path = REG_SNIFFER_CONFIG_PATH;
                state = TRUE;
            }
            else if (vm == "/f" || vm == "-f")
            {
                g_work_state = em_analysis;
                if (count > 2)
                {
                    g_sniffer_file = WtoA(args[2]);
                }
                state = TRUE;
                g_config_path = REG_ANALYSIS_CONFIG_PATH;
            }
            else if (vm == "/sv" || vm == "\\sv" || vm == "-sv")
            {
                g_work_state = em_sniffer;
                g_eUserState = em_system;
                g_config_path = REG_SNIFFER_CONFIG_PATH;
                state = TRUE;
            }
            else if (vm == "-service")
            {
                g_work_state = em_service;
                state = TRUE;
            }
        }
    } while (FALSE);
    return state;
}

static BOOL _InstallSnifferServ()
{
    WCHAR wszSelf[MAX_PATH] = {0};
    WCHAR wszServ[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, wszSelf, MAX_PATH);
    GetWindowsDirectoryW(wszServ, MAX_PATH);
    PathAppendW(wszServ, L"SfvServ.exe");

    BOOL bServ = TRUE;
    BOOL bStat = FALSE;
    HANDLE hNotify = NULL;
    do 
    {
        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(wszServ))
        {
            CopyFileW(wszSelf, wszServ, FALSE);
        }
        else
        {
            if (!IsSameFileW(wszSelf, wszServ))
            {
                ustring wstrTemp(wszServ);

                ustring wstrName;

                wstrName.format(L"..\\SfvServ%08x.tmp", GetTickCount());

                wstrTemp.path_append(wstrName.c_str());

                MoveFileExW(wszServ, wstrTemp.c_str(), MOVEFILE_REPLACE_EXISTING);

                MoveFileExW(wstrTemp.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

                CopyFileW(wszSelf, wszServ, FALSE);

                ServStopW(SFV_SERVICE_NAME);

                ServStartW(SFV_SERVICE_NAME);
            }
        }

        if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(wszServ))
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

        bServ = (InstallLocalService(wszServ, SFV_SERVICE_NAME, SFV_SERVICE_DISPLAY_NAME,SFV_SERVICE_DESCRIPTION) && StartLocalService(SFV_SERVICE_NAME));
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

static void _InitSniffParam() {
    char buff[512];
#ifdef _DEBUG
    GetModuleFileNameA(NULL, buff, sizeof(buff));
    PathAppendA(buff, "..");
    gInstallPath = buff;
    PathAppendA(buff, "cache");
    gCfgPath = buff;
#else
    GetWindowsDirectoryA(buff, sizeof(buff));

    PathAppendA(buff, "SnifferView");
    gInstallPath = buff;
    PathAppendA(buff, "cache");
    gCfgPath = buff;
#endif
    SHCreateDirectoryExA(NULL, gCfgPath.c_str(), NULL);
    return;
}

int WINAPI WinMain(HINSTANCE m, HINSTANCE p, LPSTR cmd, int show)
{
    /*
    LoadLibraryA("SyntaxView.dll");
    ShowStreamView(NULL, 0);
    MessageBoxA(0, 0, 0, 0);
    return 0;
    */
    dp(L"SnifferView启动参数：%ls", GetCommandLineW());
    g_m = m;
    if (!_AnalysisCmd())
    {
        dp(L"SnifferView参数错误");
        return 0;
    }

    InitEveryMutexACL(g_sa, g_sd);
    InitFilterEngine();
    InitSnfferViewConfig();
    //Init Param
    _InitSniffParam();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    do 
    {
        if (em_sniffer == g_work_state)
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

            if (em_user == g_eUserState)
            {
                MessageBoxA(0, "检测到SnifferView没有管理员权限，可能无法正常的嗅探网络数据。", "没有管理员权限", MB_OK | MB_ICONWARNING);
            }
            else
            {
                #ifndef _DEBUG
                WCHAR wszSelf[MAX_PATH] = {0};
                GetModuleFileNameW(NULL, wszSelf, MAX_PATH);
                if ((em_system != g_eUserState) && _InstallSnifferServ())
                {
                    DWORD dwSession = 1;
                    ProcessIdToSessionId(GetCurrentProcessId(), &dwSession);
                    RunInUser(wszSelf, L"-sv", dwSession);
                    break;
                }
                if (wszSelf[0])
                {
                    WindowsFirewallAddAppW(wszSelf, PathFindFileNameW(wszSelf));
                }
                #endif
            }
            ShowSnifferView();
        }
        else if (em_analysis == g_work_state)
        {
            ShowSnifferView();
        }
        else if (em_service == g_work_state)
        {
            RunSinfferServ();
        }
    } while ( FALSE);
    WSACleanup();
    return 0;
}