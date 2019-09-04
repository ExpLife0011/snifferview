#include <Windows.h>
#include <Shlwapi.h>

//初始化服务
BOOL InstallLocalService(LPCWSTR wszImage, LPCWSTR wszsName, LPCWSTR wszDisplayName, LPCWSTR wszDescripion)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSev = NULL;
    BOOL bStat = FALSE;
    do
    {
        if (!wszImage || !*wszImage)
        {
            break;
        }
        hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hScm)
        {
            break;
        }
        WCHAR wszDependencies[] = L"tcpip\0\0\0";
        hSev = OpenServiceW(hScm, wszsName, SERVICE_ALL_ACCESS);
        WCHAR wszCmd[MAX_PATH] = {0};
        lstrcpynW(wszCmd, wszImage, MAX_PATH);
        PathQuoteSpacesW(wszCmd);
        StrCatBuffW(wszCmd, L" -service", MAX_PATH);
        if (!hSev)
        {
            hSev = CreateServiceW(
                hScm,
                wszsName,
                wszDisplayName,
                SERVICE_ALL_ACCESS,
                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                SERVICE_AUTO_START,
                SERVICE_ERROR_NORMAL,
                wszCmd,
                NULL,
                NULL,
                wszDependencies,
                NULL,
                NULL
                );
        }
        else
        {
            ChangeServiceConfigW(
                hSev,
                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                SERVICE_AUTO_START,
                SERVICE_ERROR_NORMAL,
                wszCmd,
                NULL,
                NULL,
                wszDependencies,
                NULL,
                NULL,
                wszDisplayName
                );
        }

        if (hSev)
        {
            SERVICE_DESCRIPTIONW sdc = {(LPWSTR)wszDescripion};
            bStat = ChangeServiceConfig2W(hSev, SERVICE_CONFIG_DESCRIPTION, &sdc);
        }
    } while (FALSE);

    if (hSev)
    {
        CloseServiceHandle(hSev);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }
    return bStat;
}

//启动服务
BOOL StartLocalService(LPCWSTR wszSrvName)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSev = NULL;
    BOOL bStat = FALSE;
    do 
    {
        hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hScm)
        {
            break;
        }
        hSev = OpenServiceW(hScm, wszSrvName, SERVICE_ALL_ACCESS);
        if (!hSev)
        {
            break;
        }
        bStat = StartServiceW(hSev, 0, NULL);
        if (!bStat && ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
        {
            bStat = TRUE;
        }
    } while (FALSE);

    if (hSev)
    {
        CloseServiceHandle(hSev);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }
    return bStat;
}

//停止服务
BOOL StopLocalService(LPCWSTR wszServName)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSev = NULL;
    BOOL bStat = FALSE;
    do 
    {
        hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hScm)
        {
            break;
        }
        hSev = OpenServiceW(hScm, wszServName, SERVICE_ALL_ACCESS);
        if (!hSev)
        {
            break;
        }
        SERVICE_STATUS status;
        if (!QueryServiceStatus(hSev, &status))
        {
            break;
        }
        if (status.dwCurrentState == SERVICE_STOPPED)
        {
            bStat = TRUE;
            break;
        }

        if (status.dwCurrentState == SERVICE_STOP_PENDING || ControlService(hSev, SERVICE_CONTROL_STOP, &status))
        {
            DWORD dwCount = GetTickCount();
            while (GetTickCount() - dwCount <= 5000)
            {
                if (QueryServiceStatus(hSev, &status) && status.dwCurrentState == SERVICE_STOPPED)
                {
                    bStat = TRUE;
                }
                Sleep(500);
            }
        }
    } while (FALSE);

    if (hSev)
    {
        CloseServiceHandle(hSev);
    }
    if (hScm)
    {
        CloseServiceHandle(hScm);
    }
    return bStat;
}

BOOL RemoveLocalService(LPCWSTR wszServName)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hServ = NULL;
    BOOL bStat = FALSE;
    do 
    {
        hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hScm)
        {
            break;
        }
        hServ = OpenServiceW(hScm, wszServName, SERVICE_ALL_ACCESS);
        if (!hServ)
        {
            break;
        }
        bStat = DeleteService(hServ);
    } while (FALSE);

    if (hServ)
    {
        CloseServiceHandle(hServ);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }
    return bStat;
}

BOOL ReportLocalServStatus(SERVICE_STATUS_HANDLE hStatus, DWORD dwCurrentStat, DWORD dwWin32ExitCode)
{
    static SERVICE_STATUS s_stat =
    {
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS
    };
    if (SERVICE_START_PENDING == dwCurrentStat)
    {
        s_stat.dwControlsAccepted = 0;
    }
    else
    {
        s_stat.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
    }
    s_stat.dwCurrentState = dwCurrentStat;
    s_stat.dwWin32ExitCode = dwWin32ExitCode;
    return SetServiceStatus(hStatus, &s_stat);
}