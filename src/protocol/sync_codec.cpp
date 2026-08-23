#include "samp/protocol/sync_codec.h"

#include <algorithm>
#include <cmath>

namespace samp::protocol {
namespace {

constexpr float kQuaternionScale = 1.0f / 65535.0f;

constexpr float kDirectionScale = 2.0f / 65535.0f;

constexpr float kVelocityEpsilon = 0.00001f;

constexpr int kHealthStep = 7;
constexpr std::uint8_t kFullHealthCode = 0xF;

std::uint16_t QuantiseUnitComponent(float value, float scale) {
    const float raw = value / scale;
    const float clamped = std::clamp(raw, 0.0f, 65535.0f);
    return static_cast<std::uint16_t>(clamped + 0.5f);
}

}

std::uint8_t EncodeHealthNibble(std::uint8_t value) {
    if (value >= 100) {
        return kFullHealthCode;
    }
    if (value == 0) {
        return 0;
    }

    const int code = value / kHealthStep;
    return static_cast<std::uint8_t>(std::clamp(code, 1, kFullHealthCode - 1));
}

std::uint8_t DecodeHealthNibble(std::uint8_t nibble) {
    if (nibble == kFullHealthCode) {
        return 100;
    }
    if (nibble == 0) {
        return 0;
    }
    return static_cast<std::uint8_t>(nibble * kHealthStep);
}

void WriteCompressedQuaternion(net::BitStream& stream, const Quaternion& rotation) {
    stream.WriteBit(rotation.w < 0.0f);
    stream.WriteBit(rotation.x < 0.0f);
    stream.WriteBit(rotation.y < 0.0f);
    stream.WriteBit(rotation.z < 0.0f);

    stream.Write(QuantiseUnitComponent(std::fabs(rotation.x), kQuaternionScale));
    stream.Write(QuantiseUnitComponent(std::fabs(rotation.y), kQuaternionScale));
    stream.Write(QuantiseUnitComponent(std::fabs(rotation.z), kQuaternionScale));
}

bool ReadCompressedQuaternion(net::BitStream& stream, Quaternion& rotation) {
    bool negativeW = false;
    bool negativeX = false;
    bool negativeY = false;
    bool negativeZ = false;

    if (!stream.ReadBit(negativeW) || !stream.ReadBit(negativeX) || !stream.ReadBit(negativeY) ||
        !stream.ReadBit(negativeZ)) {
        return false;
    }

    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t z = 0;
    if (!stream.Read(x) || !stream.Read(y) || !stream.Read(z)) {
        return false;
    }

    rotation.x = static_cast<float>(x) * kQuaternionScale;
    rotation.y = static_cast<float>(y) * kQuaternionScale;
    rotation.z = static_cast<float>(z) * kQuaternionScale;

    if (negativeX) rotation.x = -rotation.x;
    if (negativeY) rotation.y = -rotation.y;
    if (negativeZ) rotation.z = -rotation.z;

    float remainder = 1.0f - rotation.x * rotation.x - rotation.y * rotation.y -
                      rotation.z * rotation.z;
    if (remainder < 0.0f) {
        remainder = 0.0f;
    }

    rotation.w = std::sqrt(remainder);
    if (negativeW) {
        rotation.w = -rotation.w;
    }

    return true;
}

void WriteCompressedVelocity(net::BitStream& stream, const Vector3& velocity) {
    const float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y +
                                      velocity.z * velocity.z);
    stream.Write(magnitude);

    if (magnitude <= kVelocityEpsilon) {
        return;
    }

    const Vector3 direction{velocity.x / magnitude, velocity.y / magnitude, velocity.z / magnitude};
    stream.Write(QuantiseUnitComponent(direction.x + 1.0f, kDirectionScale));
    stream.Write(QuantiseUnitComponent(direction.y + 1.0f, kDirectionScale));
    stream.Write(QuantiseUnitComponent(direction.z + 1.0f, kDirectionScale));
}

bool ReadCompressedVelocity(net::BitStream& stream, Vector3& velocity) {
    float magnitude = 0.0f;
    if (!stream.Read(magnitude)) {
        return false;
    }

    if (magnitude <= kVelocityEpsilon) {
        velocity = Vector3{};
        return true;
    }

    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t z = 0;
    if (!stream.Read(x) || !stream.Read(y) || !stream.Read(z)) {
        return false;
    }

    velocity.x = (static_cast<float>(x) * kDirectionScale - 1.0f) * magnitude;
    velocity.y = (static_cast<float>(y) * kDirectionScale - 1.0f) * magnitude;
    velocity.z = (static_cast<float>(z) * kDirectionScale - 1.0f) * magnitude;
    return true;
}

void WriteVehicleSync(net::BitStream& stream, std::uint16_t playerId, const VehicleSyncData& sync) {
    stream.Write(playerId);
    stream.Write(sync.vehicleId);
    stream.Write(sync.leftRightKeys);
    stream.Write(sync.upDownKeys);
    stream.Write(sync.keys);

    WriteCompressedQuaternion(stream, sync.rotation);
    stream.WriteBytes(&sync.position, sizeof(sync.position));
    WriteCompressedVelocity(stream, sync.velocity);

    stream.Write(static_cast<std::uint16_t>(sync.vehicleHealth));

    const std::uint8_t packedHealth =
        static_cast<std::uint8_t>((EncodeHealthNibble(sync.playerHealth) << 4) |
                                  EncodeHealthNibble(sync.armour));
    stream.Write(packedHealth);
    stream.Write(static_cast<std::uint8_t>(sync.weaponId));

    stream.WriteBit(sync.sirenState != 0);
    stream.WriteBit(sync.landingGearState != 0);

    const bool hasTrainSpeed = sync.trainSpeed != 0.0f;
    stream.WriteBit(hasTrainSpeed);
    if (hasTrainSpeed) {
        stream.Write(sync.trainSpeed);
    }

    const bool hasTrailer = sync.trailerId != 0;
    stream.WriteBit(hasTrailer);
    if (hasTrailer) {
        stream.Write(sync.trailerId);
    }
}

bool ReadVehicleSync(net::BitStream& stream, std::uint16_t& playerId, VehicleSyncData& sync) {
    sync = VehicleSyncData{};

    if (!stream.Read(playerId) || !stream.Read(sync.vehicleId) ||
        !stream.Read(sync.leftRightKeys) || !stream.Read(sync.upDownKeys) ||
        !stream.Read(sync.keys)) {
        return false;
    }

    if (!ReadCompressedQuaternion(stream, sync.rotation)) {
        return false;
    }
    if (!stream.ReadBytes(&sync.position, sizeof(sync.position))) {
        return false;
    }
    if (!ReadCompressedVelocity(stream, sync.velocity)) {
        return false;
    }

    std::uint16_t vehicleHealth = 0;
    if (!stream.Read(vehicleHealth)) {
        return false;
    }
    sync.vehicleHealth = static_cast<float>(vehicleHealth);

    std::uint8_t packedHealth = 0;
    if (!stream.Read(packedHealth)) {
        return false;
    }
    sync.playerHealth = DecodeHealthNibble(static_cast<std::uint8_t>(packedHealth >> 4));
    sync.armour = DecodeHealthNibble(static_cast<std::uint8_t>(packedHealth & 0x0F));

    std::uint8_t weapon = 0;
    if (!stream.Read(weapon)) {
        return false;
    }
    sync.weaponId = weapon & 0x3F;

    bool flag = false;
    if (!stream.ReadBit(flag)) {
        return false;
    }
    sync.sirenState = flag ? 1 : 0;

    if (!stream.ReadBit(flag)) {
        return false;
    }
    sync.landingGearState = flag ? 1 : 0;

    if (!stream.ReadBit(flag)) {
        return false;
    }
    if (flag && !stream.Read(sync.trainSpeed)) {
        return false;
    }

    if (!stream.ReadBit(flag)) {
        return false;
    }
    if (flag && !stream.Read(sync.trailerId)) {
        return false;
    }

    return true;
}

bool ReadPlayerSync(net::BitStream& stream, std::uint16_t& playerId, OnFootSyncData& sync) {
    sync = OnFootSyncData{};
    sync.surfingVehicleId = 0xFFFF;

    if (!stream.Read(playerId)) {
        return false;
    }

    bool present = false;
    if (!stream.ReadBit(present)) {
        return false;
    }
    if (present && !stream.Read(sync.leftRightKeys)) {
        return false;
    }

    if (!stream.ReadBit(present)) {
        return false;
    }
    if (present && !stream.Read(sync.upDownKeys)) {
        return false;
    }

    if (!stream.Read(sync.keys) || !stream.ReadBytes(&sync.position, sizeof(sync.position))) {
        return false;
    }

    if (!ReadCompressedQuaternion(stream, sync.rotation)) {
        return false;
    }

    std::uint8_t packedHealth = 0;
    if (!stream.Read(packedHealth)) {
        return false;
    }
    sync.health = DecodeHealthNibble(static_cast<std::uint8_t>(packedHealth >> 4));
    sync.armour = DecodeHealthNibble(static_cast<std::uint8_t>(packedHealth & 0x0F));

    std::uint8_t weapon = 0;
    if (!stream.Read(weapon)) {
        return false;
    }
    sync.weaponId = static_cast<std::uint8_t>(weapon & 0x3F);

    if (!stream.Read(sync.specialAction)) {
        return false;
    }

    if (!ReadCompressedVelocity(stream, sync.velocity)) {
        return false;
    }

    if (!stream.ReadBit(present)) {
        return false;
    }
    if (present) {
        if (!stream.Read(sync.surfingVehicleId) ||
            !stream.ReadBytes(&sync.surfingOffset, sizeof(sync.surfingOffset))) {
            return false;
        }
    }

    if (!stream.ReadBit(present)) {
        return false;
    }
    if (present && !stream.ReadBytes(&sync.animationId, sizeof(sync.animationId) + sizeof(sync.animationFlags))) {
        return false;
    }

    return true;
}

void WritePlayerSync(net::BitStream& stream, std::uint16_t playerId, const OnFootSyncData& sync) {
    stream.Write(playerId);

    const bool hasLeftRightKeys = sync.leftRightKeys != 0;
    stream.WriteBit(hasLeftRightKeys);
    if (hasLeftRightKeys) {
        stream.Write(sync.leftRightKeys);
    }

    const bool hasUpDownKeys = sync.upDownKeys != 0;
    stream.WriteBit(hasUpDownKeys);
    if (hasUpDownKeys) {
        stream.Write(sync.upDownKeys);
    }

    stream.Write(sync.keys);
    stream.WriteBytes(&sync.position, sizeof(sync.position));
    WriteCompressedQuaternion(stream, sync.rotation);

    const std::uint8_t packedHealth =
        static_cast<std::uint8_t>((EncodeHealthNibble(sync.health) << 4) |
                                  EncodeHealthNibble(sync.armour));
    stream.Write(packedHealth);
    stream.Write(static_cast<std::uint8_t>(sync.weaponId));
    stream.Write(sync.specialAction);
    WriteCompressedVelocity(stream, sync.velocity);

    const bool hasSurfing = sync.surfingVehicleId != 0 && sync.surfingVehicleId != 0xFFFF;
    stream.WriteBit(hasSurfing);
    if (hasSurfing) {
        stream.Write(sync.surfingVehicleId);
        stream.WriteBytes(&sync.surfingOffset, sizeof(sync.surfingOffset));
    }

    const bool hasAnimation =
        sync.animationId != 0 || sync.animationFlags != 0;
    stream.WriteBit(hasAnimation);
    if (hasAnimation) {
        stream.WriteBytes(&sync.animationId,
                          sizeof(sync.animationId) + sizeof(sync.animationFlags));
    }
}

bool ReadRelayedSync(net::BitStream& stream, std::uint16_t& playerId, void* data,
                     std::size_t size) {
    return stream.Read(playerId) && stream.ReadBytes(data, size);
}

void WriteRelayedSync(net::BitStream& stream, std::uint16_t playerId, const void* data,
                      std::size_t size) {
    stream.Write(playerId);
    stream.WriteBytes(data, size);
}

}
