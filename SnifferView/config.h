#pragma  once
#include <Windows.h>

extern mstring g_config_path;

#define  REG_SNIFFER_CONFIG_PATH    ("software\\snifferview\\sniffer\\config")
#define  REG_ANALYSIS_CONFIG_PATH   ("software\\snifferview\\analysis\\config")

VOID InitSnfferViewConfig();

VOID SaveFilterConfig();

VOID GetFilterConfig();
