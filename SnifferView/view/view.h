#pragma  once
#include <Windows.h>
#include <mstring.h>

#define MSG_UPDATE_DATA            (WM_USER + 10010)    //更新数据
#define MSG_UPDATE_SELECT          (WM_USER + 10011)    //更新用户选择
#define MSG_ACTIVE_WINDOW          (WM_USER + 20001)    //激活窗体
#define MSG_PACKETS_OVERFLOW       (WM_USER + 20005)    //封包数量溢出

#define SNIFFER_STATE_NAME         ("SnifferView - 封包嗅探")
#define SNIFFER_SUSPEND_NAME       ("SnifferView - 封包嗅探 -  已暂停")

extern mstring g_def_dir;
extern HWND g_main_view;

VOID SendUserSelectMessage(ULONG hbegin, ULONG hend, ULONG sbegin, ULONG send);

VOID WINAPI ShowSnifferView();

VOID WINAPI NoticefSnifferViewRefush();

VOID WINAPI UpdatePacketsCount();

VOID WINAPI UpdateHexSelect(DWORD sbegin, DWORD send, DWORD hbegin, DWORD hend);

//投递非实时的消息，能有效防止给窗口投递消息频率过高导致cpu过高
VOID PostDelayMessage(HWND hwnd, DWORD msg, WPARAM wp, LPARAM lp);

VOID NotifyPacketsOverFlow();