#pragma once

#include "samp/core/vector3.h"

namespace samp::protocol {

inline constexpr float kPositionDeadZone = 0.05f;

inline constexpr float kSnapDistanceXY = 8.0f;

inline constexpr float kSnapDistanceZ = 0.5f;
inline constexpr float kSnapDistanceZLarge = 2.0f;

inline constexpr int kLargeVehicleTypeFirst = 3;
inline constexpr int kLargeVehicleTypeLast = 5;

inline constexpr float kBlendFactor = 0.06f;

inline constexpr float kVelocityDeadZone = 0.01f;

enum class InterpolationAction {

    None,

    Nudge,

    Snap,
};

struct InterpolationResult {
    InterpolationAction action = InterpolationAction::None;

    Vector3 velocity;
};

constexpr float SnapDistanceZFor(int vehicleType) {
    return (vehicleType >= kLargeVehicleTypeFirst && vehicleType <= kLargeVehicleTypeLast)
               ? kSnapDistanceZLarge
               : kSnapDistanceZ;
}

InterpolationResult InterpolateTowards(const Vector3& current, const Vector3& target,
                                       const Vector3& currentVelocity, float snapDistanceZ);

}
