#include "samp/protocol/sync_structures.h"
#include "samp/network/bit_stream.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

void TestBitfieldPacking() {
    samp::protocol::OnFootSyncData sync{};
    sync.weaponId = 0x1F;
    sync.additionalKey = 0x3;

    const auto* raw = reinterpret_cast<const std::uint8_t*>(&sync);
    const std::uint8_t packed = raw[36];

    Check((packed & 0x3F) == 0x1F, "weapon id occupies the low six bits");
    Check((packed >> 6) == 0x3, "additional key occupies the top two bits");
}

void TestSyncPacketRoundTrip() {
    samp::protocol::OnFootSyncData sent{};
    sent.keys = 0x1234;
    sent.position = {10.5f, -20.25f, 3.0f};
    sent.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    sent.health = 100;
    sent.armour = 50;
    sent.weaponId = 24;
    sent.specialAction = 2;
    sent.velocity = {0.5f, 0.0f, -0.125f};
    sent.surfingVehicleId = 0xFFFF;
    sent.animationId = 1337;

    samp::net::BitStream stream;
    stream.Write(static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync));
    stream.WriteBytes(&sent, sizeof(sent));

    Check(stream.GetNumberOfBitsUsed() == 8 + 68 * 8, "sync packet is one id byte plus the payload");

    std::uint8_t packetId = 0;
    samp::protocol::OnFootSyncData received{};
    Check(stream.Read(packetId), "packet id reads back");
    Check(packetId == 0xCF, "packet id is the on-foot sync id");
    Check(stream.ReadBytes(&received, sizeof(received)), "payload reads back");
    Check(std::memcmp(&sent, &received, sizeof(sent)) == 0, "payload round-trips byte for byte");
}

void TestPassengerBitfieldPacking() {
    samp::protocol::PassengerSyncData sync{};
    sync.seatId = 0x2A;
    sync.aiming = 1;
    sync.driveBy = 1;
    sync.weaponId = 0x11;
    sync.additionalKey = 0x2;

    const auto* raw = reinterpret_cast<const std::uint8_t*>(&sync);

    Check((raw[2] & 0x3F) == 0x2A, "seat id occupies the low six bits");
    Check((raw[2] & 0x40) != 0, "aiming flag is bit six of the seat byte");
    Check((raw[2] & 0x80) != 0, "drive-by flag is the top bit of the seat byte");
    Check((raw[3] & 0x3F) == 0x11, "passenger weapon occupies the low six bits");
    Check((raw[3] >> 6) == 0x2, "passenger additional key occupies the top two bits");
}

void TestAimBitfieldPacking() {
    samp::protocol::AimSyncData sync{};
    sync.cameraFov = 0x3F;
    sync.weaponState = 0x3;

    const auto* raw = reinterpret_cast<const std::uint8_t*>(&sync);

    Check((raw[29] & 0x3F) == 0x3F, "camera fov occupies the low six bits");
    Check((raw[29] >> 6) == 0x3, "weapon state occupies the top two bits");
}

void TestPacketIdsAreDistinct() {
    using samp::protocol::PacketId;
    const PacketId ids[] = {PacketId::AimSync,        PacketId::BulletSync,
                            PacketId::PlayerSync,     PacketId::UnoccupiedSync,
                            PacketId::TrailerSync,    PacketId::PassengerSync};

    for (std::size_t i = 0; i < std::size(ids); ++i) {
        for (std::size_t j = i + 1; j < std::size(ids); ++j) {
            if (ids[i] == ids[j]) {
                Check(false, "sync packet ids are distinct");
                return;
            }
        }
    }
}

}  // namespace

int main() {
    TestBitfieldPacking();
    TestSyncPacketRoundTrip();
    TestPassengerBitfieldPacking();
    TestAimBitfieldPacking();
    TestPacketIdsAreDistinct();

    if (g_failures == 0) {
        std::printf("All sync structure tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
