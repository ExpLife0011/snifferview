#pragma  once
#include "../ComLib/mstring.h"
#include "protocol.h"

using namespace std;

void InitPacketLocker();
void ClearPacketBuffer();
void PushPacket(const char *buffer, size_t length);
BOOL PopPacket(mstring &packet);;