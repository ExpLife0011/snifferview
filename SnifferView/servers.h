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

//刷新网卡信息
VOID RefushSnifferServers();

//对所有的网卡启动嗅探
VOID StartAllSnifferServers();

//根据网卡名对某个网卡进行嗅探
BOOL StartSnifferServer(const char *name);

//暂停对某个网卡的嗅探
VOID SuspendSnifferServer(const char *name);

//停止对所有网卡的嗅探
VOID StopAllSnifferServers();

//这个网卡是否正在被嗅探
BOOL IsNetcardSniffer(const char *name);

//将所有的网卡加入嗅探队列
VOID AddAllNetcard();

//从嗅探队列删除指定网卡
VOID DeleteNetcard(const char *name);

//将指定网卡加入嗅探队列
VOID InsertNetcard(const char *name);

//初始化
VOID InitSnifferServers();
