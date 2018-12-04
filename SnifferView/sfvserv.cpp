#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include "common/common.h"
#include "common/servhlpr.h"
#include "sfvserv.h"

#define PATH_SERVICE_CACHE L"software\\snifferview\\sfvserv"
static HANDLE gs_hWorkNotify = NULL;
static SERVICE_STATUS_HANDLE gs_hStatus = NULL;
static HANDLE gs_hServiceThread = NULL;
static HANDLE gs_hServiceLeave = CreateEventW(NULL, FALSE, FALSE, NULL);

static DWORD WINAPI _ServiceHandlerEx(DWORD dwControl, DWORD dwEvent, LPVOID pEventData, LPVOID pContext)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        {
            if (!gs_hServiceThread)
            {
                ReportLocalServStatus(gs_hStatus, SERVICE_STOPPED, ERROR_SUCCESS);
                break;
            }
            ReportLocalServStatus(gs_hStatus, SERVICE_STOP_PENDING, ERROR_SUCCESS);
            SetEvent(gs_hServiceLeave);
            if (WAIT_TIMEOUT == WaitForSingleObject(gs_hServiceThread, 3000))
            {
                DWORD dwCode = 0;
                if (GetExitCodeThread(gs_hServiceThread, &dwCode) && STILL_ACTIVE == dwCode)
                {
                    TerminateThread(gs_hServiceThread, 0);
                }
            }
            CloseHandle(gs_hServiceThread);
            gs_hServiceThread = NULL;
            ReportLocalServStatus(gs_hStatus, SERVICE_STOPPED, ERROR_SUCCESS);
        }
        break;
    case  SERVICE_CONTROL_SHUTDOWN:
        break;
    case  SERVICE_CONTROL_INTERROGATE:
        {
            if (WAIT_OBJECT_0 == WaitForSingleObject(gs_hServiceThread, 0))
            {
                ReportLocalServStatus(gs_hStatus, SERVICE_STOPPED, ERROR_SUCCESS);
            }
            else
            {
                ReportLocalServStatus(gs_hStatus, SERVICE_RUNNING, ERROR_SUCCESS);
            }
        }
        break;
    default:
        break;
    }
    return ERROR_SUCCESS;
}

static VOID _RunProcess(LPCWSTR wszKey)
{
    WCHAR wszImage[1024] = {0};
    WCHAR wszCmd[1024] = {0};
    WCHAR wszSub[MAX_PATH] = {0};
    DWORD dwSize = sizeof(wszImage);
    DWORD dwSessionId = 1;
    wnsprintfW(
        wszSub,
        sizeof(wszSub) / sizeof(WCHAR),
        L"%ls\\%ls",
        PATH_SERVICE_CACHE,
        wszKey
        );
    if (!wszSub[0])
    {
        return;
    }
    SHGetValueW(
        HKEY_LOCAL_MACHINE,
        wszSub,
        L"image",
        NULL,
        wszImage,
        &dwSize
        );
    if (!wszImage[0])
    {
        return;
    }
    dwSize = sizeof(wszCmd);
    SHGetValueW(
        HKEY_LOCAL_MACHINE,
        wszSub,
        L"cmd",
        NULL,
        wszCmd,
        &dwSize
        );
    dwSize = sizeof(dwSessionId);
    SHGetValueW(
        HKEY_LOCAL_MACHINE,
        wszSub,
        L"sessionId",
        NULL,
        &dwSessionId,
        &dwSize
        );

    DWORD dwShell = 0;

    dwSize = sizeof(dwShell);

    SHGetValueW(HKEY_LOCAL_MACHINE, wszSub, L"shell", NULL, &dwShell, &dwSize);

    dp(L"image:%ls, cmd:%ls, session:%d, shell:%d", wszImage, wszCmd, dwSessionId, dwShell);

    RunInSession(wszImage, wszCmd, dwSessionId, dwShell);
}

static VOID _OnServWork()
{
    HKEY hKey = NULL;
    if (ERROR_SUCCESS != RegOpenKeyW(HKEY_LOCAL_MACHINE, PATH_SERVICE_CACHE, &hKey))
    {
        return;
    }
    DWORD dwIdex = 0;
    WCHAR wszName[MAX_PATH] = {0x00};
    DWORD dwNameLen = MAX_PATH;
    DWORD dwStatus = 0;
    list<wstring> lst;
    while (TRUE)
    {
        dwNameLen = MAX_PATH;
        wszName[0] = 0;
        dwStatus = SHEnumKeyExW(hKey, dwIdex++, wszName, &dwNameLen);
        if (wszName[0])
        {
            lst.push_back(wszName);
        }
        if (ERROR_SUCCESS != dwStatus)
        {
            break;
        }
        if (16 == lstrlenW(wszName))
        {
            _RunProcess(wszName);
        }
    }
    list<wstring>::iterator itm;
    for (itm = lst.begin() ; itm != lst.end() ; itm++)
    {
        SHDeleteKeyW(hKey, itm->c_str());
    }
    RegCloseKey(hKey);
}

static DWORD WINAPI _ServiveMain(LPVOID param)
{
    DWORD dwTickCount = 0;
    HANDLE arry[] = {gs_hWorkNotify, gs_hServiceLeave};
    DWORD dwRet = 0;
    while (TRUE)
    {
        dwRet = WaitForMultipleObjects(
            sizeof(arry) / sizeof(HANDLE),
            arry,
            FALSE,
            2000
            );
        if (WAIT_TIMEOUT == dwRet)
        {
        }

        if (WAIT_OBJECT_0 == dwRet)
        {
            _OnServWork();
        }

        if (WAIT_OBJECT_0 + 1 == dwRet)
        {
            break;
        }
    }
    return 0;
}

static VOID WINAPI _ServiceMainProc(DWORD dwArgc, LPWSTR *wszArgv)
{
    gs_hWorkNotify = CreateLowsdEvent(FALSE, FALSE, SFV_NOTIFY_NAME);
    gs_hStatus = RegisterServiceCtrlHandlerExW(SFV_SERVICE_NAME, _ServiceHandlerEx, 0);
    ReportLocalServStatus(gs_hStatus, SERVICE_START_PENDING, ERROR_SUCCESS);
    if (!gs_hServiceThread)
    {
        gs_hServiceThread = CreateThread(NULL, 0, _ServiveMain, NULL, 0, NULL);
    }
    ReportLocalServStatus(gs_hStatus, SERVICE_RUNNING, ERROR_SUCCESS);
}

VOID RunInUser(LPCWSTR wszImage, LPCWSTR wszCmd, DWORD dwSession)
{
    if (!wszImage || !*wszImage)
    {
        return;
    }

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(wszImage))
    {
        return;
    }

    static DWORD s_dwSerial = 0xffff;
    srand(GetTickCount());
    WCHAR wszSub[1024] = {0};
    WCHAR wszMagic[32] = {0};
    wnsprintfW(wszMagic, 32, L"%04X%04X%08X", rand(), rand(), s_dwSerial++);
    wnsprintfW(wszSub, 1024, L"%ls\\%ls", PATH_SERVICE_CACHE, wszMagic);
    SHSetValueW(
        HKEY_LOCAL_MACHINE,
        wszSub,
        L"image",
        REG_SZ,
        wszImage,
        (lstrlenW(wszImage) + 1) * sizeof(WCHAR)
        );
    if (wszCmd)
    {
        SHSetValueW(
            HKEY_LOCAL_MACHINE,
            wszSub,
            L"cmd",
            REG_SZ,
            wszCmd,
            (lstrlenW(wszCmd) + 1) * sizeof(WCHAR)
            );
    }

    SHSetValueW(
        HKEY_LOCAL_MACHINE,
        wszSub,
        L"sessionId",
        REG_DWORD,
        &dwSession,
        sizeof(dwSession)
        );

    DWORD dwShell = 0;

    HWND hShell = FindWindowW(L"Shell_TrayWnd", NULL);

    GetWindowThreadProcessId(hShell, &dwShell);

    if (dwShell)
    {
        SHSetValueW(HKEY_LOCAL_MACHINE, wszSub, L"shell", REG_DWORD, &dwShell, sizeof(dwShell));
    }

    HANDLE hNotify = OpenEventW(EVENT_MODIFY_STATE, FALSE, SFV_NOTIFY_NAME);
    if (hNotify)
    {
        SetEvent(hNotify);
        CloseHandle(hNotify);
    }
    return;
}

VOID RunSinfferServ()
{
    SERVICE_TABLE_ENTRYW ste[] = {
        {SFV_SERVICE_NAME, _ServiceMainProc},
        {NULL, NULL},
    };
    StartServiceCtrlDispatcherW(ste);
}