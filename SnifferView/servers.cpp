#include "servers.h"

#define NAME_127Adap "LoopAdap"

HANDLE g_adapter_lock = CreateMutexA(NULL, FALSE, NULL);
vector<AdapterMsg> g_adapters;
map<mstring, IO_SERVER *> g_servers;
mstring g_adapter_policys = "all";
set<mstring> g_sniffer_servers;

static BOOL GetAdapterByName(const char *name, AdapterMsg &out)
{
	BOOL ret = FALSE;
	vector<AdapterMsg>::iterator itm;
	LOCK_NET;
	for (itm = g_adapters.begin() ; itm != g_adapters.end() ; itm++)
	{
		if (itm->m_name == name)
		{
			out = *itm;
			ret = TRUE;
			break;
		}
	}
	UNLOCK_NET;
	return ret;
}

VOID RefushSnifferServers()
{
	LOCK_NET;
	g_adapters.clear();
	GetAdapterMsg(g_adapters);
	vector<AdapterMsg>::iterator itm;
	for (itm = g_adapters.begin() ; itm != g_adapters.end() ; itm++)
	{
		if(g_servers.find(itm->m_name) == g_servers.end())
		{
			g_servers[itm->m_name] = new IO_SERVER();
		}
	}

    g_servers[NAME_127Adap] = new IO_SERVER();
	UNLOCK_NET;
}

VOID StartAllSnifferServers()
{
	LOCK_NET;
	if (g_adapters.size() == 0)
	{
		RefushSnifferServers();
	}
	vector<AdapterMsg>::iterator itm;
	for (itm = g_adapters.begin() ; itm != g_adapters.end() ; itm++)
	{
		if (itm->m_ip != "0.0.0.0" && (g_servers.end() != g_servers.find(itm->m_name)))
		{
			g_servers[itm->m_name]->restart();
			g_servers[itm->m_name]->run(inet_addr(itm->m_ip.c_str()), 0);
		}
	}

    g_servers[NAME_127Adap]->restart();
    g_servers[NAME_127Adap]->run(inet_addr("127.0.0.1"), 0);
	UNLOCK_NET;
}

VOID StopAllSnifferServers()
{
	map<mstring, IO_SERVER *>::iterator itm;
	LOCK_NET;
	for (itm = g_servers.begin() ; itm != g_servers.end() ; itm++)
	{
		if (itm->second->is_start())
		{
			itm->second->stop();
		}
		delete itm->second;
	}
	g_servers.clear();
	g_adapters.clear();
	UNLOCK_NET;
}

BOOL StartSnifferServer(const char *name)
{
	if (!name)
	{
		return FALSE;
	}
	BOOL ret = FALSE;
	vector<AdapterMsg>::iterator itm;
	AdapterMsg m;
	LOCK_NET;
	if (g_servers.end() != g_servers.find(name))
	{
		if (GetAdapterByName(name, m) && m.m_ip != "0.0.0.0")
		{
			g_servers[name]->restart();
			ret = g_servers[name]->run(inet_addr(m.m_ip.c_str()), 0);
		}
	}
	else
	{
		RefushSnifferServers();
		if (g_servers.end() != g_servers.find(name))
		{
			if (GetAdapterByName(name, m) && m.m_ip != "0.0.0.0")
			{
				ret = g_servers[name]->run(inet_addr(m.m_ip.c_str()), 0);
			}
		}
	}
	UNLOCK_NET;
	return ret;
}

VOID SuspendSnifferServer(const char *name)
{
	if (!name)
	{
		return;
	}
	vector<AdapterMsg>::iterator itm;
	LOCK_NET;
	if (g_servers.end() != g_servers.find(name))
	{
		if (g_servers[name]->is_start())
		{
			g_servers[name]->suspend();
		}
	}
	UNLOCK_NET;
	return;
}

BOOL IsNetcardSniffer(const char *name)
{
	BOOL ret = FALSE;
	LOCK_NET;
	if (g_servers.end() != g_servers.find(name))
	{
		ret = g_servers[name]->is_sniffer();
	}
	UNLOCK_NET;
	return ret;
}

VOID InsertNetcard(const char *name)
{
	LOCK_NET;
	g_sniffer_servers.insert(name);
	if(g_adapter_policys != "all")
	{
		g_adapter_policys.clear();
		set<mstring>::iterator itm;
		int its = 0;
		for (itm = g_sniffer_servers.begin() ; itm != g_sniffer_servers.end() ; itm++)
		{
			g_adapter_policys += itm->c_str();
			g_adapter_policys += ";";
		}
	}
	UNLOCK_NET;
}

VOID DeleteNetcard(const char *name)
{
	LOCK_NET;
	if (g_sniffer_servers.end() != g_sniffer_servers.find(name))
	{
		g_sniffer_servers.erase(name);
		g_adapter_policys.clear();
		set<mstring>::iterator itm;
		for (itm = g_sniffer_servers.begin() ; itm != g_sniffer_servers.end() ; itm++)
		{
			g_adapter_policys += itm->c_str();
			g_adapter_policys += ";";
		}
	}
	UNLOCK_NET;
}

VOID AddAllNetcard()
{
	LOCK_NET;
	g_sniffer_servers.clear();
	g_adapter_policys = "all";
	RefushSnifferServers();
	vector<AdapterMsg>::iterator itm;
	for (itm = g_adapters.begin() ; itm != g_adapters.end() ; itm++)
	{
		g_sniffer_servers.insert(itm->m_name);
	}
	UNLOCK_NET;
}

VOID InitSnifferServers()
{
	LOCK_NET;
	RefushSnifferServers();
	if (g_adapter_policys == "all")
	{
		AddAllNetcard();
		StartAllSnifferServers();
	}
	else
	{
		list<mstring> ss;
		GetCutBufferList(g_adapter_policys.c_str(), ';', ss);
		list<mstring>::iterator itm;
		for (itm = ss.begin() ; itm != ss.end() ; itm++)
		{
			StartSnifferServer(itm->c_str());
			g_sniffer_servers.insert(itm->c_str());
		}
	}
	UNLOCK_NET;
}