#pragma once

#include "samp/protocol/samp_client.h"
#include "samp/protocol/sync_structures.h"
#include "samp/vehicle/vehicle_pool.h"

namespace samp::vehicle {

class VehicleSyncCache final : public protocol::SyncSink {
public:
    void OnUnoccupiedSync(std::uint16_t playerId,
                          const protocol::UnoccupiedSyncData& state) override;

    void OnTrailerSync(std::uint16_t playerId,
                       const protocol::TrailerSyncData& state) override;

    void SetDownstream(protocol::SyncSink* sink);

    const protocol::UnoccupiedSyncData* GetUnoccupied(std::uint16_t vehicleId) const;

    std::uint16_t GetUnoccupiedSender(std::uint16_t vehicleId) const;

    const protocol::TrailerSyncData* GetTrailer(std::uint16_t trailerId) const;

    std::uint16_t GetTrailerSender(std::uint16_t trailerId) const;

    void Clear();

private:
    static constexpr std::size_t kSlotCount = VehiclePool::kCapacity;

    protocol::UnoccupiedSyncData unoccupied_[kSlotCount]{};
    protocol::TrailerSyncData trailers_[kSlotCount]{};
    std::uint16_t unoccupiedSender_[kSlotCount]{};
    std::uint16_t trailerSender_[kSlotCount]{};
    bool hasUnoccupied_[kSlotCount]{};
    bool hasTrailer_[kSlotCount]{};
    protocol::SyncSink* downstream_ = nullptr;
};

}
