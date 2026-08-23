#include "samp/protocol/samp_client.h"

#include "samp/protocol/sync_codec.h"

namespace samp::protocol {

namespace {

template <typename Sync>
bool SendThrottledSync(net::Transport& transport, SyncThrottle& throttle, PacketId id,
                       const Sync& state, std::uint32_t nowMs) {
    net::BitStream packet;
    if (!BuildSyncPacket(throttle, id, state, nowMs, packet)) {
        return false;
    }
    return transport.Send(packet, net::kSyncDelivery);
}

bool MustArrive(RpcId id) {
    switch (id) {
        case RpcId::DialogResponse:
        case RpcId::GiveTakeDamage:
        case RpcId::GiveActorDamage:
            return true;
        default:
            return false;
    }
}

}

void SyncSink::OnPlayerSync(std::uint16_t, const OnFootSyncData&) {}
void SyncSink::OnVehicleSync(std::uint16_t, const VehicleSyncData&) {}
void SyncSink::OnPassengerSync(std::uint16_t, const PassengerSyncData&) {}
void SyncSink::OnTrailerSync(std::uint16_t, const TrailerSyncData&) {}
void SyncSink::OnUnoccupiedSync(std::uint16_t, const UnoccupiedSyncData&) {}
void SyncSink::OnAimSync(std::uint16_t, const AimSyncData&) {}
void SyncSink::OnBulletSync(std::uint16_t, const BulletSyncData&) {}

SampClient::SampClient(net::Transport& transport) : transport_(transport) {

    dispatcher_.On<InitGame>(RpcId::InitGame, &ReadInitGame,
                             [this](const InitGame& message) {
                                 initGame_ = message;
                                 gameInitialised_ = true;
                             });
}

void SampClient::SetPacketHandler(PacketHandler handler) {
    packetHandler_ = std::move(handler);
}

void SampClient::SetSyncSink(SyncSink* sink) {
    syncSink_ = sink;
}

const net::DeliveryOptions& SampClient::DeliveryClassFor(RpcId id) {
    return MustArrive(id) ? net::kReliableRpcDelivery : net::kRpcDelivery;
}

bool SampClient::SendRpc(RpcId id, const net::BitStream& payload) {
    return transport_.SendRpc(ToByte(id), payload, DeliveryClassFor(id));
}

bool SampClient::SendClassSelection(std::int32_t classIndex) {
    net::BitStream payload;
    payload.Write(classIndex);
    return SendRpc(RpcId::RequestClass, payload);
}

bool SampClient::SendSpawnRequest() {
    return SendRpc(RpcId::RequestSpawn, net::BitStream{});
}

bool SampClient::SendPlayerSync(const OnFootSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.player, PacketId::PlayerSync, state, nowMs);
}

bool SampClient::SendVehicleSync(const VehicleSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.vehicle, PacketId::VehicleSync, state, nowMs);
}

bool SampClient::SendPassengerSync(const PassengerSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.passenger, PacketId::PassengerSync, state,
                             nowMs);
}

bool SampClient::SendTrailerSync(const TrailerSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.trailer, PacketId::TrailerSync, state, nowMs);
}

bool SampClient::SendUnoccupiedSync(const UnoccupiedSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.unoccupied, PacketId::UnoccupiedSync, state,
                             nowMs);
}

bool SampClient::SendAimSync(const AimSyncData& state, std::uint32_t nowMs) {
    return SendThrottledSync(transport_, throttles_.aim, PacketId::AimSync, state, nowMs);
}

bool SampClient::SendBulletSync(const BulletSyncData& shot) {
    net::BitStream packet;
    packet.Write(static_cast<std::uint8_t>(PacketId::BulletSync));
    packet.WriteBytes(&shot, sizeof(shot));
    return transport_.Send(packet, net::kBulletSyncDelivery);
}

void SampClient::ResetSyncThrottles() {
    throttles_ = OutgoingSyncThrottles{};
}

void SampClient::OnPacket(const net::BitStream& packet) {

    net::BitStream stream(packet.GetData(), packet.GetNumberOfBytesUsed(), true);
    PacketHeader header;
    if (!ReadPacketHeader(stream, header)) {
        return;
    }

    RouteSync(header, stream);
}

void SampClient::RouteSync(const PacketHeader& header, net::BitStream& body) {
    if (syncSink_ != nullptr) {
        switch (static_cast<PacketId>(header.id)) {
            case PacketId::PlayerSync: {
                std::uint16_t playerId = 0;
                OnFootSyncData state{};
                if (ReadPlayerSync(body, playerId, state)) {
                    syncSink_->OnPlayerSync(playerId, state);
                }
                return;
            }
            case PacketId::VehicleSync: {
                std::uint16_t playerId = 0;
                VehicleSyncData state{};
                if (ReadVehicleSync(body, playerId, state)) {
                    syncSink_->OnVehicleSync(playerId, state);
                }
                return;
            }
            case PacketId::PassengerSync: {
                std::uint16_t playerId = 0;
                PassengerSyncData state{};
                if (ReadRelayedSync(body, playerId, &state, sizeof(state))) {
                    syncSink_->OnPassengerSync(playerId, state);
                }
                return;
            }
            case PacketId::TrailerSync: {
                std::uint16_t playerId = 0;
                TrailerSyncData state{};
                if (ReadRelayedSync(body, playerId, &state, sizeof(state))) {
                    syncSink_->OnTrailerSync(playerId, state);
                }
                return;
            }
            case PacketId::UnoccupiedSync: {
                std::uint16_t playerId = 0;
                UnoccupiedSyncData state{};
                if (ReadRelayedSync(body, playerId, &state, sizeof(state))) {
                    syncSink_->OnUnoccupiedSync(playerId, state);
                }
                return;
            }
            case PacketId::AimSync: {
                std::uint16_t playerId = 0;
                AimSyncData state{};
                if (ReadRelayedSync(body, playerId, &state, sizeof(state))) {
                    syncSink_->OnAimSync(playerId, state);
                }
                return;
            }
            case PacketId::BulletSync: {
                std::uint16_t playerId = 0;
                BulletSyncData state{};
                if (ReadRelayedSync(body, playerId, &state, sizeof(state))) {
                    syncSink_->OnBulletSync(playerId, state);
                }
                return;
            }
            default:
                break;
        }
    }

    if (packetHandler_) {
        packetHandler_(header, body);
    }
}

void SampClient::OnRpc(std::uint8_t id, const net::BitStream& payload) {
    net::BitStream stream(payload.GetData(), payload.GetNumberOfBytesUsed(), true);
    dispatcher_.Dispatch(static_cast<RpcId>(id), stream);
}

void SampClient::OnConnected() {
    connected_ = true;
}

void SampClient::OnDisconnected() {
    connected_ = false;
    ResetSyncThrottles();
}

}
