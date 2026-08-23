#include "samp/vehicle/vehicle_pool.h"

namespace samp::vehicle {

const VehicleRecord* VehiclePool::Get(std::uint16_t vehicleId) const {
    if (!protocol::IsValidVehicleId(vehicleId)) {
        return nullptr;
    }
    const VehicleRecord& record = vehicles_[vehicleId];
    return record.inUse ? &record : nullptr;
}

bool VehiclePool::Add(std::uint16_t vehicleId, std::int32_t modelId, std::uint8_t colour1,
                      std::uint8_t colour2) {
    if (!protocol::IsValidVehicleId(vehicleId)) {
        return false;
    }

    VehicleRecord& record = vehicles_[vehicleId];
    record.inUse = true;
    record.modelId = modelId;
    record.colour1 = colour1;
    record.colour2 = colour2;
    return true;
}

bool VehiclePool::Remove(std::uint16_t vehicleId) {
    if (!protocol::IsValidVehicleId(vehicleId)) {
        return false;
    }

    VehicleRecord& record = vehicles_[vehicleId];
    if (!record.inUse) {
        return false;
    }

    record = VehicleRecord{};
    return true;
}

}
