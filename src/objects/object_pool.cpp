#include "samp/object/object_pool.h"

namespace samp::object {

const ObjectRecord* ObjectPool::Get(std::uint16_t objectId) const {
    if (!protocol::IsValidObjectId(objectId)) {
        return nullptr;
    }
    const ObjectRecord& record = objects_[objectId];
    return record.inUse ? &record : nullptr;
}

bool ObjectPool::Add(std::uint16_t objectId, const protocol::CreateObject& creation) {
    if (!protocol::IsValidObjectId(objectId)) {
        return false;
    }

    ObjectRecord& record = objects_[objectId];
    record.inUse = true;
    record.modelId = creation.modelId;
    record.position = creation.position;
    record.rotation = creation.rotation;
    record.attachedVehicleId = creation.attachedVehicleId;
    record.attachedObjectId = creation.attachedObjectId;
    record.noCameraCollision = creation.noCameraCollision;
    return true;
}

bool ObjectPool::Remove(std::uint16_t objectId) {
    if (!protocol::IsValidObjectId(objectId)) {
        return false;
    }

    ObjectRecord& record = objects_[objectId];
    if (!record.inUse) {
        return false;
    }

    record = ObjectRecord{};
    return true;
}

}
