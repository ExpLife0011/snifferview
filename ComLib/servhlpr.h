#ifndef SERVHLPR_H_H_
#define SERVHLPR_H_H_
#include <Windows.h>

//��ʼ������
BOOL InstallLocalService(LPCWSTR wszImage, LPCWSTR wszsName, LPCWSTR wszDisplayName, LPCWSTR wszDescripion);

//��������
BOOL StartLocalService(LPCWSTR wszSrvName);

//ֹͣ����
BOOL StopLocalService(LPCWSTR wszServName);

//�Ƴ�����
BOOL RemoveLocalService(LPCWSTR wszServName);

BOOL ReportLocalServStatus(SERVICE_STATUS_HANDLE hStatus, DWORD dwCurrentStat, DWORD dwWin32ExitCode);
#endif //SERVHLPR_H_H_