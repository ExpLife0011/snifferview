#ifndef SERVHLPR_H_H_
#define SERVHLPR_H_H_
#include <Windows.h>

//初始化服务
BOOL InstallLocalService(LPCWSTR wszImage, LPCWSTR wszsName, LPCWSTR wszDisplayName, LPCWSTR wszDescripion);

//启动服务
BOOL StartLocalService(LPCWSTR wszSrvName);

//停止服务
BOOL StopLocalService(LPCWSTR wszServName);

//移除服务
BOOL RemoveLocalService(LPCWSTR wszServName);

BOOL ReportLocalServStatus(SERVICE_STATUS_HANDLE hStatus, DWORD dwCurrentStat, DWORD dwWin32ExitCode);
#endif //SERVHLPR_H_H_