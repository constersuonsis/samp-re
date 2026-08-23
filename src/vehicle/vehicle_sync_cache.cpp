#include "samp/vehicle/vehicle_sync_cache.h"

namespace samp::vehicle {

void VehicleSyncCache::OnUnoccupiedSync(std::uint16_t playerId,
                                        const protocol::UnoccupiedSyncData& state) {
    if (!protocol::IsValidVehicleId(state.vehicleId)) {
        return;
    }
    unoccupied_[state.vehicleId] = state;
    unoccupiedSender_[state.vehicleId] = playerId;
    hasUnoccupied_[state.vehicleId] = true;
    if (downstream_ != nullptr) {
        downstream_->OnUnoccupiedSync(playerId, state);
    }
}

void VehicleSyncCache::OnTrailerSync(std::uint16_t playerId,
                                     const protocol::TrailerSyncData& state) {
    if (!protocol::IsValidVehicleId(state.trailerId)) {
        return;
    }
    trailers_[state.trailerId] = state;
    trailerSender_[state.trailerId] = playerId;
    hasTrailer_[state.trailerId] = true;
    if (downstream_ != nullptr) {
        downstream_->OnTrailerSync(playerId, state);
    }
}

void VehicleSyncCache::SetDownstream(protocol::SyncSink* sink) {
    downstream_ = sink;
}

const protocol::UnoccupiedSyncData* VehicleSyncCache::GetUnoccupied(
    std::uint16_t vehicleId) const {
    if (!protocol::IsValidVehicleId(vehicleId) || !hasUnoccupied_[vehicleId]) {
        return nullptr;
    }
    return &unoccupied_[vehicleId];
}

std::uint16_t VehicleSyncCache::GetUnoccupiedSender(std::uint16_t vehicleId) const {
    return protocol::IsValidVehicleId(vehicleId) ? unoccupiedSender_[vehicleId] : 0;
}

const protocol::TrailerSyncData* VehicleSyncCache::GetTrailer(std::uint16_t trailerId) const {
    if (!protocol::IsValidVehicleId(trailerId) || !hasTrailer_[trailerId]) {
        return nullptr;
    }
    return &trailers_[trailerId];
}

std::uint16_t VehicleSyncCache::GetTrailerSender(std::uint16_t trailerId) const {
    return protocol::IsValidVehicleId(trailerId) ? trailerSender_[trailerId] : 0;
}

void VehicleSyncCache::Clear() {
    for (std::size_t i = 0; i < kSlotCount; ++i) {
        hasUnoccupied_[i] = false;
        hasTrailer_[i] = false;
    }
}

}
