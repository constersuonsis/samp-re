#pragma once

#include "samp/core/vector3.h"

namespace samp::protocol {

/// Difference below which an axis is treated as already in place.
inline constexpr float kPositionDeadZone = 0.05f;

/// Horizontal distance past which catching up gradually is abandoned.
inline constexpr float kSnapDistanceXY = 8.0f;

/// Vertical distance past which catching up is abandoned. Larger vehicles are
/// given more room before they are snapped.
inline constexpr float kSnapDistanceZ = 0.5f;
inline constexpr float kSnapDistanceZLarge = 2.0f;

/// Vehicle types allowed the larger vertical tolerance.
inline constexpr int kLargeVehicleTypeFirst = 3;
inline constexpr int kLargeVehicleTypeLast = 5;

/// Share of the remaining distance folded into velocity each step.
inline constexpr float kBlendFactor = 0.06f;

/// Velocity below which a nudge is not worth applying.
inline constexpr float kVelocityDeadZone = 0.01f;

/// What should be done to bring a vehicle towards its synced position.
enum class InterpolationAction {
    /// Already close enough, or the nudge would be too small to matter.
    None,
    /// Apply the returned velocity and let the physics carry it.
    Nudge,
    /// Too far out to blend; place it at the target outright.
    Snap,
};

struct InterpolationResult {
    InterpolationAction action = InterpolationAction::None;
    /// Only meaningful for a nudge.
    Vector3 velocity;
};

/// Vertical snap distance for a vehicle type.
constexpr float SnapDistanceZFor(int vehicleType) {
    return (vehicleType >= kLargeVehicleTypeFirst && vehicleType <= kLargeVehicleTypeLast)
               ? kSnapDistanceZLarge
               : kSnapDistanceZ;
}

/// Works out how to close the gap between where a vehicle is and where the
/// server says it should be.
///
/// Catching up is done by adding to velocity rather than by moving the vehicle,
/// so it keeps colliding with the world on the way. That only holds while the
/// gap is small: past the snap distances the vehicle is placed outright,
/// because blending across a large gap would drive it through whatever lies
/// between.
///
/// Each axis is considered separately: an axis already in place contributes
/// nothing even while another is being corrected.
InterpolationResult InterpolateTowards(const Vector3& current, const Vector3& target,
                                       const Vector3& currentVelocity, float snapDistanceZ);

}  // namespace samp::protocol
