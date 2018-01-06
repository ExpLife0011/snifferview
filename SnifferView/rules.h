#pragma  once
#include "filter.h"

typedef BOOL (WINAPI *PRegMatchFun)(IN const char *rule, OUT FilterRules &result);

VOID WINAPI RegistFIlterRules();

PRegMatchFun WINAPI GetRulesHandle(const char *filter);