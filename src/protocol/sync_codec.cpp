#include "samp/protocol/sync_codec.h"

#include <algorithm>
#include <cmath>

namespace samp::protocol {
namespace {

/// A quaternion component is mapped onto the full 16-bit range.
constexpr float kQuaternionScale = 1.0f / 65535.0f;

/// A direction component covers [-1, 1] across the same range.
constexpr float kDirectionScale = 2.0f / 65535.0f;

/// Below this length a velocity is treated as standing still and its direction
/// is not sent at all.
constexpr float kVelocityEpsilon = 0.00001f;

/// Health steps up in this many points per nibble code.
constexpr int kHealthStep = 7;
constexpr std::uint8_t kFullHealthCode = 0xF;

std::uint16_t QuantiseUnitComponent(float value, float scale) {
    const float raw = value / scale;
    const float clamped = std::clamp(raw, 0.0f, 65535.0f);
    return static_cast<std::uint16_t>(clamped + 0.5f);
}

}  // namespace

std::uint8_t EncodeHealthNibble(std::uint8_t value) {
    if (value >= 100) {
        return kFullHealthCode;
    }
    if (value == 0) {
        return 0;
    }
    // Code zero is reserved for "dead", so anything below one full step is
    // rounded up rather than down: a player on their last few points must not
    // arrive on the other side reading as zero.
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

    // Only the magnitudes travel; w is rebuilt from the unit-length constraint.
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

    // The direction is normalised first so each component fits [-1, 1].
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

    // Train speed and trailer are optional and, unlike in the decoded state,
    // the speed comes first on the wire.
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

}  // namespace samp::protocol
