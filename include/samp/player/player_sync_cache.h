#pragma once

#include "samp/player/player_pool.h"
#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/samp_client.h"
#include "samp/protocol/sync_structures.h"

namespace samp::player {

class PlayerSyncCache final : public protocol::SyncSink {
public:
    void OnPlayerSync(std::uint16_t playerId,
                      const protocol::OnFootSyncData& state) override;

    void OnVehicleSync(std::uint16_t playerId,
                       const protocol::VehicleSyncData& state) override;

    void OnPassengerSync(std::uint16_t playerId,
                         const protocol::PassengerSyncData& state) override;

    void SetDownstream(protocol::SyncSink* sink);

    const protocol::OnFootSyncData* GetOnFoot(std::uint16_t playerId) const;

    const protocol::VehicleSyncData* GetDrivenVehicle(std::uint16_t playerId) const;

    const protocol::PassengerSyncData* GetPassengerSeat(std::uint16_t playerId) const;

    bool IsDriving(std::uint16_t playerId) const;

    void Clear();

private:
    static constexpr std::size_t kSlotCount =
        static_cast<std::size_t>(protocol::kMaxPlayerId) + 1;

    protocol::OnFootSyncData onFoot_[kSlotCount]{};
    protocol::VehicleSyncData driven_[kSlotCount]{};
    protocol::PassengerSyncData passenger_[kSlotCount]{};
    bool hasOnFoot_[kSlotCount]{};
    bool hasDriven_[kSlotCount]{};
    bool hasPassenger_[kSlotCount]{};
    bool driving_[kSlotCount]{};
    protocol::SyncSink* downstream_ = nullptr;
};

}
