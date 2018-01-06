#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include "servers.h"
#include "config.h"
#include "filter.h"
#include "global.h"
#include "view/view.h"

mstring g_config_path;

VOID InitSnfferViewConfig()
{
	HKEY hkey = NULL;
    DWORD dwError = 0;
	if (ERROR_SUCCESS != (dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_config_path.c_str(), 0, KEY_ALL_ACCESS, &hkey)))
	{
		DWORD flag = REG_CREATED_NEW_KEY;
		RegCreateKeyExA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), 0, 0, 0, NULL, &g_sa, &hkey, &flag);
		SaveFilterConfig();
	}
	else
	{
		GetFilterConfig();
	}

	InitFilterConfig();
	if (!IsDirectoryExist(g_def_dir.c_str()))
	{
		mstring vv;
		GetModuleFileNameA(NULL, vv.alloc(MAX_PATH), MAX_PATH);
		vv.setbuffer();
		vv.repsub("/", "\\");
		int v = vv.rfind('\\');
		if (mstring::npos == v)
		{
			vv.clear();
		}
		else
		{
			int a = vv.size() - v;
			vv.erase(v, vv.size() - v);
		}
		g_def_dir = vv;
		SaveFilterConfig();
	}

	if (hkey)
	{
		RegCloseKey(hkey);
	}
}

VOID SaveFilterConfig()
{
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "encode", REG_DWORD, &g_net_mark, sizeof(g_net_mark));
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "igcase", REG_DWORD, &g_str_mark, sizeof(g_str_mark));
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "showstring", REG_SZ, g_show_string.c_str(), g_show_string.size());
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "filterstring", REG_SZ, g_filter_string.c_str(), g_filter_string.size());
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "hexhight", REG_DWORD, &g_hex_hight, sizeof(DWORD));
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "netcard", REG_SZ, g_adapter_policys.c_str(), g_adapter_policys.size());
	SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "defdir", REG_SZ, g_def_dir.c_str(), g_def_dir.size());

    ULONGLONG llCount = GetMaxPacketCount();
    SHSetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "maxcount", REG_QWORD, &llCount, sizeof(llCount));
}

VOID GetFilterConfig()
{
	DWORD length = sizeof(g_net_mark);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "encode", NULL, &g_net_mark, &length);
	length = sizeof(g_str_mark);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "igcase", NULL, &g_str_mark, &length);
	char sm[1024] = {0x00};
	length = sizeof(sm);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "showstring", NULL, sm, &length);
	g_show_string = sm;
	length = sizeof(sm);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "filterstring", NULL, sm, &length);
	g_filter_string = sm;
	length = sizeof(g_hex_hight);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "hexhight", NULL, &g_hex_hight, &length);
	length = sizeof(sm);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "netcard", NULL, sm, &length);
	g_adapter_policys = sm;
	length = sizeof(sm);
	SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "defdir", NULL, sm, &length);
	g_def_dir = sm;

    ULONGLONG llCount = 0;
    DWORD dwCount = sizeof(llCount);
    SHGetValueA(HKEY_LOCAL_MACHINE, g_config_path.c_str(), "maxcount", NULL, &llCount, &dwCount);

    if (llCount > 0)
    {
        SetMaxPacketCount(llCount);
    }
}