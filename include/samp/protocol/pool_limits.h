#pragma once

#include <cstdint>

namespace samp::protocol {

// Every id that arrives from the network is used to index a fixed array, so it
// has to be checked first. The bounds below are gathered in one place because
// they are deliberately not uniform: the sizes differ, and so does whether the
// top value itself is accepted. Guessing either way reads past the end of an
// array or silently drops the last slot, so each pool keeps its own predicate.
//
//   pool         limit   top value accepted
//   players      1004    yes
//   vehicles     2000    no
//   objects      1000    yes
//   actors       1000    no
//   3D labels    2048    no
//   textdraws    2304    no
//   pickups      4096    no

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

/// Highest weapon the game recognises. Handlers drop anything above it rather
/// than passing it into the weapon tables.
inline constexpr std::uint32_t kMaxWeaponId = 46;

constexpr bool IsValidWeaponId(std::uint32_t id) {
    return id <= kMaxWeaponId;
}

}  // namespace samp::protocol
