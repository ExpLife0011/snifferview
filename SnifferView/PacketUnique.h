/*
packet unique get without cache. 2019/06/19
*/
#pragma once
#include "analysis.h"
#include "common/LockBase.h"
#include "common/mstring.h"

class CPacketUnique {
public:
    static CPacketUnique *GetInst();
    bool GetUnique(PacketContent &msg) const;
    static BOOL WINAPI PacketInit(IN OUT PacketContent &msg);

private:
    CPacketUnique();
    virtual ~CPacketUnique();
    bool ParserProtocol(PacketContent &msg) const;

    std::mstring GetUniqueWithDirection(const PacketContent &msg) const;
    std::mstring GetUdpUnique(const PacketContent &packet) const;
    std::mstring GetTcpUnique(const PacketContent &packet) const;
    std::mstring GetTcpUniqueWithDirection(const PacketContent &packet) const;
    std::mstring GetIcmpUnique(const PacketContent &packet) const;
};