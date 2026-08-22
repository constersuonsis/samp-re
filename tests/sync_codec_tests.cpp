#include "samp/protocol/sync_codec.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

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

void TestHealthNibble() {
    using samp::protocol::DecodeHealthNibble;
    using samp::protocol::EncodeHealthNibble;

    Check(DecodeHealthNibble(EncodeHealthNibble(100)) == 100, "full health survives quantisation");
    Check(DecodeHealthNibble(EncodeHealthNibble(0)) == 0, "zero health survives quantisation");
    Check(EncodeHealthNibble(150) == 0xF, "values above full clamp to the top code");

    // Everything in between lands on a multiple of seven, and a player who is
    // still alive must never quantise down to zero.
    for (int value = 1; value < 100; ++value) {
        const std::uint8_t decoded =
            DecodeHealthNibble(EncodeHealthNibble(static_cast<std::uint8_t>(value)));

        if (decoded == 0) {
            Check(false, "a living player never quantises to zero health");
            break;
        }
        if (std::abs(decoded - value) >= 7) {
            Check(false, "intermediate health stays within one quantisation step");
            break;
        }
    }
}

void TestQuaternionRoundTrip() {
    samp::net::BitStream stream;
    const samp::Quaternion sent{0.5f, -0.5f, 0.5f, -0.5f};
    samp::protocol::WriteCompressedQuaternion(stream, sent);

    samp::Quaternion received{};
    Check(samp::protocol::ReadCompressedQuaternion(stream, received), "quaternion reads back");
    Check(NearlyEqual(sent.w, received.w, 0.001f), "quaternion w round-trips");
    Check(NearlyEqual(sent.x, received.x, 0.001f), "quaternion x round-trips");
    Check(NearlyEqual(sent.y, received.y, 0.001f), "quaternion y round-trips");
    Check(NearlyEqual(sent.z, received.z, 0.001f), "quaternion z round-trips");

    // Four sign bits plus three 16-bit components.
    Check(stream.GetNumberOfBitsUsed() == 4 + 48, "quaternion costs 52 bits");
}

void TestVelocityRoundTrip() {
    samp::net::BitStream stream;
    const samp::Vector3 sent{1.5f, -2.25f, 0.75f};
    samp::protocol::WriteCompressedVelocity(stream, sent);

    samp::Vector3 received{};
    Check(samp::protocol::ReadCompressedVelocity(stream, received), "velocity reads back");
    Check(NearlyEqual(sent.x, received.x, 0.01f), "velocity x round-trips");
    Check(NearlyEqual(sent.y, received.y, 0.01f), "velocity y round-trips");
    Check(NearlyEqual(sent.z, received.z, 0.01f), "velocity z round-trips");
}

void TestStandingStillSkipsDirection() {
    samp::net::BitStream stream;
    samp::protocol::WriteCompressedVelocity(stream, samp::Vector3{0.0f, 0.0f, 0.0f});
    Check(stream.GetNumberOfBitsUsed() == 32, "a still vehicle sends only the magnitude");

    samp::Vector3 received{1.0f, 1.0f, 1.0f};
    Check(samp::protocol::ReadCompressedVelocity(stream, received), "still velocity reads back");
    Check(received.x == 0.0f && received.y == 0.0f && received.z == 0.0f, "still velocity is zero");
}

void TestVehicleSyncRoundTrip() {
    samp::protocol::VehicleSyncData sent{};
    sent.vehicleId = 42;
    sent.leftRightKeys = 0x0102;
    sent.upDownKeys = 0x0304;
    sent.keys = 0x0506;
    sent.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    sent.position = {1024.5f, -2048.25f, 15.0f};
    sent.velocity = {0.25f, 0.5f, -0.125f};
    sent.vehicleHealth = 850.0f;
    sent.playerHealth = 100;
    sent.armour = 49;
    sent.weaponId = 31;
    sent.sirenState = 1;
    sent.landingGearState = 0;
    sent.trailerId = 7;
    sent.trainSpeed = 0.0f;

    samp::net::BitStream stream;
    samp::protocol::WriteVehicleSync(stream, 3, sent);

    std::uint16_t playerId = 0;
    samp::protocol::VehicleSyncData received{};
    Check(samp::protocol::ReadVehicleSync(stream, playerId, received), "vehicle sync reads back");

    Check(playerId == 3, "player id round-trips");
    Check(received.vehicleId == 42, "vehicle id round-trips");
    Check(received.keys == 0x0506, "keys round-trip");
    Check(received.position.x == 1024.5f && received.position.z == 15.0f,
          "position round-trips exactly");
    Check(received.vehicleHealth == 850.0f, "vehicle health round-trips");
    Check(received.playerHealth == 100, "driver health round-trips");
    Check(received.armour == 49, "driver armour round-trips");
    Check(received.weaponId == 31, "weapon id round-trips");
    Check(received.sirenState == 1, "siren round-trips");
    Check(received.trailerId == 7, "trailer round-trips");
    Check(received.trainSpeed == 0.0f, "absent train speed stays zero");
    Check(NearlyEqual(received.velocity.y, 0.5f, 0.01f), "vehicle velocity round-trips");
}

void TestTruncatedPacketIsRejected() {
    samp::net::BitStream stream;
    stream.Write<std::uint16_t>(1);
    stream.Write<std::uint16_t>(2);

    std::uint16_t playerId = 0;
    samp::protocol::VehicleSyncData received{};
    Check(!samp::protocol::ReadVehicleSync(stream, playerId, received),
          "a truncated packet is rejected instead of reading garbage");
}

}  // namespace

int main() {
    TestHealthNibble();
    TestQuaternionRoundTrip();
    TestVelocityRoundTrip();
    TestStandingStillSkipsDirection();
    TestVehicleSyncRoundTrip();
    TestTruncatedPacketIsRejected();

    if (g_failures == 0) {
        std::printf("All sync codec tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
