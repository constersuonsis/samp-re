#pragma once

#include "samp/network/transport.h"

#include <functional>
#include <string>

namespace samp::net {

using LogSink = std::function<void(const std::string& line)>;

class PacketLogger final : public ConnectionHandler {
public:
    PacketLogger(ConnectionHandler& downstream, LogSink sink);

    void SetEnabled(bool enabled);

    void OnPacket(const BitStream& packet) override;

    void OnRpc(std::uint8_t id, const BitStream& payload) override;

    void OnConnected() override;

    void OnDisconnected() override;

private:
    void Write(const std::string& line);

    ConnectionHandler& downstream_;
    LogSink sink_;
    bool enabled_ = true;
};

}
