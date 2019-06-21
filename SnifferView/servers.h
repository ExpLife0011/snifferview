#pragma  once
#include <WinSock2.h>
#include <Windows.h>
#include <set>
#include "global.h"
#include "iocp_server.h"

using namespace std;

extern set<mstring> g_sniffer_servers;
extern HANDLE g_adapter_lock;
extern mstring g_adapter_policys;
extern vector<AdapterMsg> g_adapters;
extern map<mstring, IO_SERVER *> g_servers;

#define  LOCK_NET		WaitForSingleObject(g_adapter_lock, INFINITE)
#define	UNLOCK_NET	ReleaseMutex(g_adapter_lock)

//ˢ��������Ϣ
VOID RefushSnifferServers();

//�����е�����������̽
VOID StartAllSnifferServers();

//������������ĳ������������̽
BOOL StartSnifferServer(const char *name);

//��ͣ��ĳ����������̽
VOID SuspendSnifferServer(const char *name);

//ֹͣ��������������̽
VOID StopAllSnifferServers();

//��������Ƿ����ڱ���̽
BOOL IsNetcardSniffer(const char *name);

//�����е�����������̽����
VOID AddAllNetcard();

//����̽����ɾ��ָ������
VOID DeleteNetcard(const char *name);

//��ָ������������̽����
VOID InsertNetcard(const char *name);

//��ʼ��
VOID InitSnifferServers();
