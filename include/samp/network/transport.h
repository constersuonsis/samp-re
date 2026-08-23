#pragma once

#include "samp/network/bit_stream.h"

#include <cstdint>

namespace samp::net {

enum class Priority : std::uint8_t {
    System = 0,
    High = 1,
    Medium = 2,
    Low = 3,
};

enum class Reliability : std::uint8_t {

    Unreliable = 6,

    UnreliableSequenced = 7,

    Reliable = 8,

    ReliableOrdered = 9,
};

struct DeliveryOptions {
    Priority priority = Priority::High;
    Reliability reliability = Reliability::Reliable;
    std::uint8_t orderingChannel = 0;
};

inline constexpr DeliveryOptions kSyncDelivery{Priority::High, Reliability::UnreliableSequenced, 1};

inline constexpr DeliveryOptions kBulletSyncDelivery{Priority::High,
                                                     Reliability::UnreliableSequenced, 0};

inline constexpr DeliveryOptions kRpcDelivery{Priority::High, Reliability::Reliable, 0};

inline constexpr DeliveryOptions kReliableRpcDelivery{Priority::High,
                                                      Reliability::ReliableOrdered, 0};

class Transport {
public:
    virtual ~Transport() = default;

    virtual bool Send(const BitStream& packet, const DeliveryOptions& options) = 0;

    virtual bool SendRpc(std::uint8_t rpcId, const BitStream& payload,
                         const DeliveryOptions& options) = 0;

    virtual void Disconnect(std::uint32_t lingerMs) = 0;

    virtual bool IsConnected() const = 0;
};

class ConnectionHandler {
public:
    virtual ~ConnectionHandler() = default;

    virtual void OnPacket(const BitStream& packet) = 0;

    virtual void OnRpc(std::uint8_t id, const BitStream& payload) = 0;

    virtual void OnConnected() {}

    virtual void OnDisconnected() {}
};

}
