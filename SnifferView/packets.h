#pragma  once
#include <mstring.h>
#include "protocol.h"

using namespace std;

VOID WINAPI ClearPacketBuffer();

VOID WINAPI PushPacket(const char *buffer, size_t length);

BOOL WINAPI PopPacket(mstring &packet);