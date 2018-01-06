#pragma once
#include <Windows.h>
#include "analysis.h"

VOID WINAPI ClearConnect();

//链接初始化接口，每个链接都要首先经过这个接口来进行初始化
BOOL WINAPI ConnectInit(IN OUT PacketContent &msg);