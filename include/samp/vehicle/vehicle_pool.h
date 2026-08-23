#pragma once

#include "samp/protocol/pool_limits.h"
#include "samp/protocol/rpc_payloads.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::vehicle {

struct VehicleRecord {
    bool inUse = false;
    std::int32_t modelId = 0;

    std::uint8_t colour1 = protocol::kNoVehicleColour;
    std::uint8_t colour2 = protocol::kNoVehicleColour;
};

class VehiclePool {
public:
    static constexpr std::size_t kCapacity =
        static_cast<std::size_t>(protocol::kVehiclePoolSize);

    bool Add(std::uint16_t vehicleId, std::int32_t modelId, std::uint8_t colour1,
             std::uint8_t colour2);

    bool Remove(std::uint16_t vehicleId);

    const VehicleRecord* Get(std::uint16_t vehicleId) const;

private:
    std::array<VehicleRecord, kCapacity> vehicles_{};
};

}
