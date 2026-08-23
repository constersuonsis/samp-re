#pragma once

#include <cstdint>

namespace samp::protocol {

inline constexpr std::uint16_t kMaxPlayerId = 1004;
inline constexpr std::uint16_t kVehiclePoolSize = 2000;
inline constexpr std::uint16_t kMaxObjectId = 1000;
inline constexpr std::uint16_t kActorPoolSize = 1000;
inline constexpr std::uint16_t k3DTextLabelPoolSize = 2048;
inline constexpr std::uint16_t kTextDrawPoolSize = 2304;
inline constexpr std::uint32_t kPickupPoolSize = 4096;

constexpr bool IsValidPlayerId(std::uint16_t id) {
    return id <= kMaxPlayerId;
}

constexpr bool IsValidVehicleId(std::uint16_t id) {
    return id < kVehiclePoolSize;
}

constexpr bool IsValidObjectId(std::uint16_t id) {
    return id <= kMaxObjectId;
}

constexpr bool IsValidActorId(std::uint16_t id) {
    return id < kActorPoolSize;
}

constexpr bool IsValid3DTextLabelId(std::uint16_t id) {
    return id < k3DTextLabelPoolSize;
}

constexpr bool IsValidTextDrawId(std::uint16_t id) {
    return id < kTextDrawPoolSize;
}

constexpr bool IsValidPickupSlot(std::uint32_t slot) {
    return slot < kPickupPoolSize;
}

inline constexpr std::uint32_t kMaxWeaponId = 46;

constexpr bool IsValidWeaponId(std::uint32_t id) {
    return id <= kMaxWeaponId;
}

}
