#pragma once

#include "samp/core/vector3.h"

#include <cstddef>
#include <cstdint>

namespace samp::protocol {

/// Packet identifiers carried as the first byte of a sync payload.
enum class PacketId : std::uint8_t {
    /// Marks a packet that carries a 32-bit send time ahead of the real id.
    Timestamp = 0x28,

    AimSync = 0xCB,
    BulletSync = 0xCE,
    PlayerSync = 0xCF,
    UnoccupiedSync = 0xD1,
    TrailerSync = 0xD2,
    PassengerSync = 0xD3,
};

#pragma pack(push, 1)

/// Payload of an on-foot sync packet, sent unmodified as a block of bytes.
///
/// The client only puts this on the wire when a field actually changed or half
/// a second has passed since the last send, so the layout has to stay
/// byte-exact for the comparison against the previous frame to work.
struct OnFootSyncData {
    std::uint16_t leftRightKeys;
    std::uint16_t upDownKeys;
    std::uint16_t keys;

    Vector3 position;
    Quaternion rotation;

    std::uint8_t health;
    std::uint8_t armour;

    /// Weapon occupies the low six bits; the top two carry the extra key state
    /// that did not fit into `keys`.
    std::uint8_t weaponId : 6;
    std::uint8_t additionalKey : 2;

    std::uint8_t specialAction;

    Vector3 velocity;

    /// Position relative to the vehicle or object the player is standing on.
    /// Zeroed when not surfing.
    Vector3 surfingOffset;
    std::uint16_t surfingVehicleId;

    std::uint16_t animationId;
    std::uint16_t animationFlags;
};

/// Payload of an aim sync packet: where the camera is and what it looks at.
struct AimSyncData {
    std::uint8_t cameraMode;
    Vector3 cameraFront;
    Vector3 cameraPosition;
    float aimZ;

    std::uint8_t cameraFov : 6;
    std::uint8_t weaponState : 2;

    std::uint8_t cameraZoom;
};

/// Payload of a passenger sync packet, sent while riding as a non-driver.
struct PassengerSyncData {
    std::uint16_t vehicleId;

    std::uint8_t seatId : 6;
    /// While this is set the client also raises its aim sync rate from one
    /// second to a tenth of a second.
    std::uint8_t aiming : 1;
    /// Set while leaning out of the window with a weapon drawn.
    std::uint8_t driveBy : 1;

    std::uint8_t weaponId : 6;
    std::uint8_t additionalKey : 2;

    std::uint8_t health;
    std::uint8_t armour;

    std::uint16_t leftRightKeys;
    std::uint16_t upDownKeys;
    std::uint16_t keys;

    Vector3 position;
};

/// Payload of a trailer sync packet, sent by whoever tows the trailer.
struct TrailerSyncData {
    std::uint16_t trailerId;
    Vector3 position;
    Quaternion rotation;
    Vector3 velocity;
    Vector3 turnVelocity;
};

/// Payload of an unoccupied vehicle sync packet, sent by the player the server
/// made responsible for a driverless vehicle.
struct UnoccupiedSyncData {
    std::uint16_t vehicleId;
    std::uint8_t seatId;

    /// First and second rows of the vehicle matrix; the third is derived.
    Vector3 roll;
    Vector3 direction;

    Vector3 position;
    Vector3 velocity;
    Vector3 turnVelocity;
    float vehicleHealth;
};

/// What a bullet hit, reported alongside the shot itself.
enum class BulletHitType : std::uint8_t {
    None = 0,
    Player = 1,
    Vehicle = 2,
    Object = 3,
};

/// Payload of a bullet sync packet.
struct BulletSyncData {
    std::uint8_t hitType;
    std::uint16_t hitId;

    Vector3 origin;
    Vector3 target;
    /// Impact point, relative to the entity that was hit when there was one.
    Vector3 centreOfHit;

    std::uint8_t weaponId;
};

/// Decoded state of a player driving a vehicle.
///
/// Unlike the on-foot payload this never travels as a raw block of bytes: the
/// wire form is bit-packed and several fields are quantised, so it always goes
/// through the codec in sync_codec.h. The layout still has to stay exact
/// because the rest of the client reads it by offset.
struct VehicleSyncData {
    std::uint16_t vehicleId;

    std::uint16_t leftRightKeys;
    std::uint16_t upDownKeys;
    std::uint16_t keys;

    Quaternion rotation;
    Vector3 position;
    Vector3 velocity;

    float vehicleHealth;

    /// Both values are quantised to a nibble on the wire, so they come back in
    /// steps of seven with 100 as a special case.
    std::uint8_t playerHealth;
    std::uint8_t armour;

    std::uint8_t weaponId : 6;
    std::uint8_t unused : 2;

    std::uint8_t sirenState;
    std::uint8_t landingGearState;

    std::uint16_t trailerId;
    float trainSpeed;
};

#pragma pack(pop)

static_assert(sizeof(OnFootSyncData) == 68, "on-foot sync payload is 68 bytes on the wire");
static_assert(offsetof(OnFootSyncData, keys) == 4, "keys field offset");
static_assert(offsetof(OnFootSyncData, position) == 6, "position field offset");
static_assert(offsetof(OnFootSyncData, rotation) == 18, "rotation field offset");
static_assert(offsetof(OnFootSyncData, health) == 34, "health field offset");
static_assert(offsetof(OnFootSyncData, armour) == 35, "armour field offset");
static_assert(offsetof(OnFootSyncData, specialAction) == 37, "special action field offset");
static_assert(offsetof(OnFootSyncData, velocity) == 38, "velocity field offset");
static_assert(offsetof(OnFootSyncData, surfingOffset) == 50, "surfing offset field offset");
static_assert(offsetof(OnFootSyncData, surfingVehicleId) == 62, "surfing vehicle field offset");
static_assert(offsetof(OnFootSyncData, animationId) == 64, "animation id field offset");

static_assert(sizeof(AimSyncData) == 31, "aim sync payload is 31 bytes on the wire");
static_assert(offsetof(AimSyncData, cameraFront) == 1, "camera front field offset");
static_assert(offsetof(AimSyncData, cameraPosition) == 13, "camera position field offset");
static_assert(offsetof(AimSyncData, aimZ) == 25, "aim height field offset");
static_assert(offsetof(AimSyncData, cameraZoom) == 30, "camera zoom field offset");

static_assert(sizeof(PassengerSyncData) == 24, "passenger sync payload is 24 bytes on the wire");
static_assert(offsetof(PassengerSyncData, health) == 4, "passenger health field offset");
static_assert(offsetof(PassengerSyncData, armour) == 5, "passenger armour field offset");
static_assert(offsetof(PassengerSyncData, leftRightKeys) == 6, "passenger keys field offset");
static_assert(offsetof(PassengerSyncData, position) == 12, "passenger position field offset");

static_assert(sizeof(TrailerSyncData) == 54, "trailer sync payload is 54 bytes on the wire");
static_assert(offsetof(TrailerSyncData, position) == 2, "trailer position field offset");
static_assert(offsetof(TrailerSyncData, rotation) == 14, "trailer rotation field offset");
static_assert(offsetof(TrailerSyncData, velocity) == 30, "trailer velocity field offset");
static_assert(offsetof(TrailerSyncData, turnVelocity) == 42, "trailer turn velocity field offset");

static_assert(sizeof(UnoccupiedSyncData) == 67, "unoccupied sync payload is 67 bytes on the wire");
static_assert(offsetof(UnoccupiedSyncData, roll) == 3, "unoccupied roll field offset");
static_assert(offsetof(UnoccupiedSyncData, direction) == 15, "unoccupied direction field offset");
static_assert(offsetof(UnoccupiedSyncData, position) == 27, "unoccupied position field offset");
static_assert(offsetof(UnoccupiedSyncData, velocity) == 39, "unoccupied velocity field offset");
static_assert(offsetof(UnoccupiedSyncData, vehicleHealth) == 63, "unoccupied health field offset");

static_assert(sizeof(BulletSyncData) == 40, "bullet sync payload is 40 bytes on the wire");
static_assert(offsetof(BulletSyncData, origin) == 3, "bullet origin field offset");
static_assert(offsetof(BulletSyncData, target) == 15, "bullet target field offset");
static_assert(offsetof(BulletSyncData, centreOfHit) == 27, "bullet impact field offset");
static_assert(offsetof(BulletSyncData, weaponId) == 39, "bullet weapon field offset");

static_assert(sizeof(VehicleSyncData) == 63, "vehicle sync state is 63 bytes");
static_assert(offsetof(VehicleSyncData, keys) == 6, "vehicle keys field offset");
static_assert(offsetof(VehicleSyncData, rotation) == 8, "vehicle rotation field offset");
static_assert(offsetof(VehicleSyncData, position) == 24, "vehicle position field offset");
static_assert(offsetof(VehicleSyncData, velocity) == 36, "vehicle velocity field offset");
static_assert(offsetof(VehicleSyncData, vehicleHealth) == 48, "vehicle health field offset");
static_assert(offsetof(VehicleSyncData, playerHealth) == 52, "driver health field offset");
static_assert(offsetof(VehicleSyncData, armour) == 53, "driver armour field offset");
static_assert(offsetof(VehicleSyncData, sirenState) == 55, "siren field offset");
static_assert(offsetof(VehicleSyncData, landingGearState) == 56, "landing gear field offset");
static_assert(offsetof(VehicleSyncData, trailerId) == 57, "trailer field offset");
static_assert(offsetof(VehicleSyncData, trainSpeed) == 59, "train speed field offset");

/// The client resends an unchanged sync packet once this much time has passed.
inline constexpr std::uint32_t kSyncResendIntervalMs = 500;

}  // namespace samp::protocol
