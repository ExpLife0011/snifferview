#pragma once
#include <Windows.h>
#include "analysis.h"

VOID WINAPI ClearConnect();

//���ӳ�ʼ���ӿڣ�ÿ�����Ӷ�Ҫ���Ⱦ�������ӿ������г�ʼ��
BOOL WINAPI ConnectInit(IN OUT PacketContent &msg);