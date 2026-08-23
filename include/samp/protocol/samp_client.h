#pragma once

#include "samp/network/transport.h"
#include "samp/protocol/packet_reader.h"
#include "samp/protocol/rpc_dispatcher.h"
#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/sync_sender.h"
#include "samp/protocol/sync_structures.h"
#include "samp/protocol/sync_throttle.h"

#include <functional>

namespace samp::protocol {

class SyncSink {
public:
    virtual ~SyncSink() = default;

    virtual void OnPlayerSync(std::uint16_t playerId, const OnFootSyncData& state);
    virtual void OnVehicleSync(std::uint16_t playerId, const VehicleSyncData& state);
    virtual void OnPassengerSync(std::uint16_t playerId, const PassengerSyncData& state);
    virtual void OnTrailerSync(std::uint16_t playerId, const TrailerSyncData& state);
    virtual void OnUnoccupiedSync(std::uint16_t playerId, const UnoccupiedSyncData& state);
    virtual void OnAimSync(std::uint16_t playerId, const AimSyncData& state);
    virtual void OnBulletSync(std::uint16_t playerId, const BulletSyncData& state);
};

class SampClient final : public net::ConnectionHandler {
public:

    using PacketHandler = std::function<void(const PacketHeader&, net::BitStream&)>;

    explicit SampClient(net::Transport& transport);

    void SetPacketHandler(PacketHandler handler);

    void SetSyncSink(SyncSink* sink);

    RpcDispatcher& Dispatcher() { return dispatcher_; }
    const RpcDispatcher& Dispatcher() const { return dispatcher_; }

    bool IsConnected() const { return connected_; }

    bool GameInitialised() const { return gameInitialised_; }

    const InitGame& ServerConfig() const { return initGame_; }

    std::uint16_t LocalPlayerId() const { return initGame_.playerId; }

    bool SendRpc(RpcId id, const net::BitStream& payload);

    bool SendClassSelection(std::int32_t classIndex);

    bool SendSpawnRequest();

    bool SendPlayerSync(const OnFootSyncData& state, std::uint32_t nowMs);

    bool SendVehicleSync(const VehicleSyncData& state, std::uint32_t nowMs);

    bool SendPassengerSync(const PassengerSyncData& state, std::uint32_t nowMs);

    bool SendTrailerSync(const TrailerSyncData& state, std::uint32_t nowMs);

    bool SendUnoccupiedSync(const UnoccupiedSyncData& state, std::uint32_t nowMs);

    bool SendAimSync(const AimSyncData& state, std::uint32_t nowMs);

    bool SendBulletSync(const BulletSyncData& shot);

    void ResetSyncThrottles();

    void OnPacket(const net::BitStream& packet) override;

    void OnRpc(std::uint8_t id, const net::BitStream& payload) override;

    void OnConnected() override;

    void OnDisconnected() override;

private:
    static const net::DeliveryOptions& DeliveryClassFor(RpcId id);

    void RouteSync(const PacketHeader& header, net::BitStream& body);

    struct OutgoingSyncThrottles {
        SyncThrottle player;
        SyncThrottle vehicle;
        SyncThrottle passenger;
        SyncThrottle trailer;
        SyncThrottle unoccupied;
        SyncThrottle aim;
    };

    net::Transport& transport_;
    OutgoingSyncThrottles throttles_;
    RpcDispatcher dispatcher_;
    PacketHandler packetHandler_;
    SyncSink* syncSink_ = nullptr;
    InitGame initGame_;
    bool gameInitialised_ = false;
    bool connected_ = false;
};

}
