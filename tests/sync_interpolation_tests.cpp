#include "samp/protocol/sync_interpolation.h"

#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

bool NearlyEqual(float a, float b, float tolerance) {
    return std::fabs(a - b) <= tolerance;
}

using samp::protocol::InterpolateTowards;
using samp::protocol::InterpolationAction;

const samp::Vector3 kStill{0.0f, 0.0f, 0.0f};

void TestAlreadyInPlaceDoesNothing() {
    const auto result = InterpolateTowards({10.0f, 10.0f, 10.0f}, {10.02f, 9.98f, 10.01f}, kStill,
                                           samp::protocol::kSnapDistanceZ);
    Check(result.action == InterpolationAction::None,
          "a gap inside the dead zone needs no correction");
}

void TestSmallGapNudges() {
    const auto result = InterpolateTowards({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, kStill,
                                           samp::protocol::kSnapDistanceZ);
    Check(result.action == InterpolationAction::Nudge, "a small gap is closed by nudging");
    Check(NearlyEqual(result.velocity.x, samp::protocol::kBlendFactor, 0.0001f),
          "the nudge is the blend factor of the remaining distance");
    Check(result.velocity.y == 0.0f && result.velocity.z == 0.0f,
          "axes already in place contribute nothing");
}

void TestLargeHorizontalGapSnaps() {
    const auto result =
        InterpolateTowards({0.0f, 0.0f, 0.0f}, {samp::protocol::kSnapDistanceXY + 0.1f, 0.0f, 0.0f},
                           kStill, samp::protocol::kSnapDistanceZ);
    Check(result.action == InterpolationAction::Snap,
          "a gap past the horizontal limit is snapped rather than blended");
}

void TestVerticalLimitDependsOnVehicleType() {
    const samp::Vector3 origin{0.0f, 0.0f, 0.0f};
    const samp::Vector3 above{0.0f, 0.0f, 1.0f};

    const auto ordinary =
        InterpolateTowards(origin, above, kStill, samp::protocol::SnapDistanceZFor(0));
    Check(ordinary.action == InterpolationAction::Snap,
          "an ordinary vehicle snaps at one metre of height");

    const auto large = InterpolateTowards(
        origin, above, kStill, samp::protocol::SnapDistanceZFor(samp::protocol::kLargeVehicleTypeFirst));
    Check(large.action == InterpolationAction::Nudge,
          "a large vehicle is allowed to blend the same gap");
}

void TestVehicleTypeBoundaries() {
    using samp::protocol::SnapDistanceZFor;

    Check(SnapDistanceZFor(samp::protocol::kLargeVehicleTypeFirst - 1) ==
              samp::protocol::kSnapDistanceZ,
          "the type below the range gets the ordinary limit");
    Check(SnapDistanceZFor(samp::protocol::kLargeVehicleTypeLast) ==
              samp::protocol::kSnapDistanceZLarge,
          "the last type of the range gets the larger limit");
    Check(SnapDistanceZFor(samp::protocol::kLargeVehicleTypeLast + 1) ==
              samp::protocol::kSnapDistanceZ,
          "the type above the range gets the ordinary limit again");
}

void TestExistingVelocityIsKept() {

    const samp::Vector3 moving{2.0f, 0.0f, 0.0f};
    const auto result = InterpolateTowards({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, moving,
                                           samp::protocol::kSnapDistanceZ);

    Check(result.action == InterpolationAction::Nudge, "the gap is nudged");
    Check(NearlyEqual(result.velocity.x, 2.0f + samp::protocol::kBlendFactor, 0.0001f),
          "the correction adds to the existing velocity rather than replacing it");
}

void TestNegligibleResultIsSkipped() {

    const float gap = 0.1f;
    const samp::Vector3 opposing{-gap * samp::protocol::kBlendFactor, 0.0f, 0.0f};

    const auto result = InterpolateTowards({0.0f, 0.0f, 0.0f}, {gap, 0.0f, 0.0f}, opposing,
                                           samp::protocol::kSnapDistanceZ);
    Check(result.action == InterpolationAction::None,
          "a correction that leaves no meaningful velocity is skipped");
}

void TestSnapWinsOverDeadZoneOnOtherAxes() {

    const auto result = InterpolateTowards({0.0f, 0.0f, 0.0f}, {0.0f, 20.0f, 0.0f}, kStill,
                                           samp::protocol::kSnapDistanceZ);
    Check(result.action == InterpolationAction::Snap,
          "a single distant axis snaps the whole vehicle");
}

}

int main() {
    TestAlreadyInPlaceDoesNothing();
    TestSmallGapNudges();
    TestLargeHorizontalGapSnaps();
    TestVerticalLimitDependsOnVehicleType();
    TestVehicleTypeBoundaries();
    TestExistingVelocityIsKept();
    TestNegligibleResultIsSkipped();
    TestSnapWinsOverDeadZoneOnOtherAxes();

    if (g_failures == 0) {
        std::printf("All interpolation tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
