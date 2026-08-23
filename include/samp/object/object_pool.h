#pragma once

#include "samp/core/vector3.h"
#include "samp/protocol/pool_limits.h"
#include "samp/protocol/rpc_payloads.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::object {

struct ObjectRecord {
    bool inUse = false;
    std::int32_t modelId = 0;
    Vector3 position;
    Vector3 rotation;

    std::uint16_t attachedVehicleId = protocol::kNoAttachment;
    std::uint16_t attachedObjectId = protocol::kNoAttachment;
    bool noCameraCollision = false;

    bool IsAttached() const {
        return attachedVehicleId != protocol::kNoAttachment ||
               attachedObjectId != protocol::kNoAttachment;
    }
};

class ObjectPool {
public:
    static constexpr std::size_t kCapacity =
        static_cast<std::size_t>(protocol::kMaxObjectId) + 1;

    bool Add(std::uint16_t objectId, const protocol::CreateObject& creation);

    bool Remove(std::uint16_t objectId);

    const ObjectRecord* Get(std::uint16_t objectId) const;

private:
    std::array<ObjectRecord, kCapacity> objects_{};
};

}
