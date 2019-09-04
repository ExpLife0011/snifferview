#ifndef WINDOWS_FIREWALL_INCLUDE
#define WINDOWS_FIREWALL_INCLUDE

#include <windows.h>

#if UNICODE || _UNICODE
#  define WindowsFirewallAddApp         WindowsFirewallAddAppW
#else
#  define WindowsFirewallAddApp         WindowsFirewallAddAppA
#endif

//增加防火墙例外规则
BOOL WINAPI WindowsFirewallAddAppA(LPCSTR lpImagePath, LPCSTR lpfwName);
BOOL WINAPI WindowsFirewallAddAppW(LPCWSTR lpImagePath, LPCWSTR lpfwName);

#endif
