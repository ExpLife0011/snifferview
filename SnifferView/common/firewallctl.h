#ifndef WINDOWS_FIREWALL_INCLUDE
#define WINDOWS_FIREWALL_INCLUDE

#include <windows.h>

#if UNICODE || _UNICODE
#  define WindowsFirewallAddApp         WindowsFirewallAddAppW
#else
#  define WindowsFirewallAddApp         WindowsFirewallAddAppA
#endif

//���ӷ���ǽ�������
BOOL WINAPI WindowsFirewallAddAppA(LPCSTR lpImagePath, LPCSTR lpfwName);
BOOL WINAPI WindowsFirewallAddAppW(LPCWSTR lpImagePath, LPCWSTR lpfwName);

#endif
