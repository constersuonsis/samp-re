#pragma once

#include "samp/network/transport.h"

#include <cstdint>
#include <memory>

namespace samp::net {

class RakNetClient final : public Transport {
public:
    RakNetClient();
    ~RakNetClient() override;

    RakNetClient(const RakNetClient&) = delete;
    RakNetClient& operator=(const RakNetClient&) = delete;

    bool Connect(const char* host, std::uint16_t serverPort, std::uint16_t clientPort,
                 const char* password = nullptr);

    void SetHandler(ConnectionHandler* handler);

    void Pump();

    bool Send(const BitStream& packet, const DeliveryOptions& options) override;

    bool SendRpc(std::uint8_t rpcId, const BitStream& payload,
                 const DeliveryOptions& options) override;

    void Disconnect(std::uint32_t lingerMs) override;

    bool IsConnected() const override;

private:
    class Peer;
    std::unique_ptr<Peer> peer_;
};

}
