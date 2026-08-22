#include "samp/protocol/sync_interpolation.h"

#include <cmath>

namespace samp::protocol {

InterpolationResult InterpolateTowards(const Vector3& current, const Vector3& target,
                                       const Vector3& currentVelocity, float snapDistanceZ) {
    const float deltaX = target.x - current.x;
    const float deltaY = target.y - current.y;
    const float deltaZ = target.z - current.z;

    const float distanceX = std::fabs(deltaX);
    const float distanceY = std::fabs(deltaY);
    const float distanceZ = std::fabs(deltaZ);

    InterpolationResult result;

    if (distanceX <= kPositionDeadZone && distanceY <= kPositionDeadZone &&
        distanceZ <= kPositionDeadZone) {
        return result;
    }

    if (distanceX > kSnapDistanceXY || distanceY > kSnapDistanceXY || distanceZ > snapDistanceZ) {
        result.action = InterpolationAction::Snap;
        return result;
    }

    result.velocity = currentVelocity;
    if (distanceX > kPositionDeadZone) {
        result.velocity.x += deltaX * kBlendFactor;
    }
    if (distanceY > kPositionDeadZone) {
        result.velocity.y += deltaY * kBlendFactor;
    }
    if (distanceZ > kPositionDeadZone) {
        result.velocity.z += deltaZ * kBlendFactor;
    }

    // The test is on the resulting velocity, not on the correction, so a
    // vehicle already moving keeps its momentum applied.
    if (std::fabs(result.velocity.x) > kVelocityDeadZone ||
        std::fabs(result.velocity.y) > kVelocityDeadZone ||
        std::fabs(result.velocity.z) > kVelocityDeadZone) {
        result.action = InterpolationAction::Nudge;
    }

    return result;
}

}  // namespace samp::protocol
