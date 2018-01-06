#pragma  once
#include <Windows.h>
#include <mstring.h>

#define MSG_UPDATE_DATA            (WM_USER + 10010)    //��������
#define MSG_UPDATE_SELECT          (WM_USER + 10011)    //�����û�ѡ��
#define MSG_ACTIVE_WINDOW          (WM_USER + 20001)    //�����
#define MSG_PACKETS_OVERFLOW       (WM_USER + 20005)    //����������

#define SNIFFER_STATE_NAME         ("SnifferView - �����̽")
#define SNIFFER_SUSPEND_NAME       ("SnifferView - �����̽ -  ����ͣ")

extern mstring g_def_dir;
extern HWND g_main_view;

VOID SendUserSelectMessage(ULONG hbegin, ULONG hend, ULONG sbegin, ULONG send);

VOID WINAPI ShowSnifferView();

VOID WINAPI NoticefSnifferViewRefush();

VOID WINAPI UpdatePacketsCount();

VOID WINAPI UpdateHexSelect(DWORD sbegin, DWORD send, DWORD hbegin, DWORD hend);

//Ͷ�ݷ�ʵʱ����Ϣ������Ч��ֹ������Ͷ����ϢƵ�ʹ��ߵ���cpu����
VOID PostDelayMessage(HWND hwnd, DWORD msg, WPARAM wp, LPARAM lp);

VOID NotifyPacketsOverFlow();