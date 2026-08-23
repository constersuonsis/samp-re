#include "samp/player/player_sync_cache.h"

namespace samp::player {

void PlayerSyncCache::OnPlayerSync(std::uint16_t playerId,
                                   const protocol::OnFootSyncData& state) {
    if (!protocol::IsValidPlayerId(playerId)) {
        return;
    }
    onFoot_[playerId] = state;
    hasOnFoot_[playerId] = true;
    driving_[playerId] = false;
    if (downstream_ != nullptr) {
        downstream_->OnPlayerSync(playerId, state);
    }
}

void PlayerSyncCache::OnVehicleSync(std::uint16_t playerId,
                                    const protocol::VehicleSyncData& state) {
    if (!protocol::IsValidPlayerId(playerId)) {
        return;
    }
    driven_[playerId] = state;
    hasDriven_[playerId] = true;
    driving_[playerId] = true;
    if (downstream_ != nullptr) {
        downstream_->OnVehicleSync(playerId, state);
    }
}

void PlayerSyncCache::OnPassengerSync(std::uint16_t playerId,
                                      const protocol::PassengerSyncData& state) {
    if (!protocol::IsValidPlayerId(playerId)) {
        return;
    }
    passenger_[playerId] = state;
    hasPassenger_[playerId] = true;
    if (downstream_ != nullptr) {
        downstream_->OnPassengerSync(playerId, state);
    }
}

void PlayerSyncCache::SetDownstream(protocol::SyncSink* sink) {
    downstream_ = sink;
}

const protocol::OnFootSyncData* PlayerSyncCache::GetOnFoot(std::uint16_t playerId) const {
    if (!protocol::IsValidPlayerId(playerId) || !hasOnFoot_[playerId]) {
        return nullptr;
    }
    return &onFoot_[playerId];
}

const protocol::VehicleSyncData* PlayerSyncCache::GetDrivenVehicle(
    std::uint16_t playerId) const {
    if (!protocol::IsValidPlayerId(playerId) || !hasDriven_[playerId]) {
        return nullptr;
    }
    return &driven_[playerId];
}

const protocol::PassengerSyncData* PlayerSyncCache::GetPassengerSeat(
    std::uint16_t playerId) const {
    if (!protocol::IsValidPlayerId(playerId) || !hasPassenger_[playerId]) {
        return nullptr;
    }
    return &passenger_[playerId];
}

bool PlayerSyncCache::IsDriving(std::uint16_t playerId) const {
    return protocol::IsValidPlayerId(playerId) && driving_[playerId];
}

void PlayerSyncCache::Clear() {
    for (std::size_t i = 0; i < kSlotCount; ++i) {
        hasOnFoot_[i] = false;
        hasDriven_[i] = false;
        hasPassenger_[i] = false;
        driving_[i] = false;
    }
}

}
