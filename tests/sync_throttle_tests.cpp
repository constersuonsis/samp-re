#include "samp/protocol/sync_throttle.h"

#include "samp/protocol/sync_sender.h"
#include "samp/protocol/sync_structures.h"

#include <cstdio>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

bool Offer(samp::protocol::SyncThrottle& throttle, const samp::protocol::OnFootSyncData& sync,
           std::uint32_t nowMs) {
    return throttle.ShouldSend(&sync, sizeof(sync), nowMs);
}

void TestFirstPayloadAlwaysGoes() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    Check(!throttle.HasSent(), "a fresh throttle has sent nothing");
    Check(Offer(throttle, sync, 0), "the first payload is always sent");
    Check(throttle.HasSent(), "the throttle remembers it sent");
}

void TestUnchangedPayloadIsHeldBack() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};
    sync.health = 100;

    Check(Offer(throttle, sync, 1000), "the first payload is sent");
    Check(!Offer(throttle, sync, 1100), "an identical payload is held back");
    Check(!Offer(throttle, sync, 1500),
          "an identical payload is still held back at the interval");
}

void TestUnchangedPayloadGoesAfterTheInterval() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    Check(Offer(throttle, sync, 1000), "the first payload is sent");
    Check(Offer(throttle, sync, 1000 + samp::protocol::SyncThrottle::kResendIntervalMs + 1),
          "a motionless player is resent once the interval passes");
}

void TestAnyChangeGoesImmediately() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    Check(Offer(throttle, sync, 1000), "the first payload is sent");

    sync.armour = 1;
    Check(Offer(throttle, sync, 1001), "a changed payload goes out at once");

    Check(!Offer(throttle, sync, 1002), "the changed payload is now the baseline");
}

void TestSmallestPossibleChangeIsNoticed() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    Check(Offer(throttle, sync, 0), "the first payload is sent");

    sync.animationFlags = 1;
    Check(Offer(throttle, sync, 1), "a change in the final field is noticed");
}

void TestClockWrapDoesNotStallSending() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    const std::uint32_t beforeWrap = 0xFFFFFF00;
    Check(Offer(throttle, sync, beforeWrap), "the first payload is sent");

    const std::uint32_t afterWrap = beforeWrap + samp::protocol::SyncThrottle::kResendIntervalMs + 1;
    Check(afterWrap < beforeWrap, "the test really does straddle the wrap");
    Check(Offer(throttle, sync, afterWrap),
          "the resend still happens when the clock wraps between sends");
}

void TestResetForgetsTheBaseline() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    Check(Offer(throttle, sync, 1000), "the first payload is sent");
    Check(!Offer(throttle, sync, 1001), "the identical payload is held back");

    throttle.Reset();
    Check(!throttle.HasSent(), "resetting clears the sent flag");
    Check(Offer(throttle, sync, 1002), "the same payload is sent again after a reset");
}

void TestBuiltPacketShape() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};
    sync.health = 100;

    samp::net::BitStream packet;
    Check(samp::protocol::BuildSyncPacket(throttle, samp::protocol::PacketId::PlayerSync, sync, 0,
                                          packet),
          "the first payload produces a packet");
    Check(packet.GetNumberOfBitsUsed() == (1 + 68) * 8,
          "the packet is an id byte followed by the payload");

    std::uint8_t id = 0;
    samp::protocol::OnFootSyncData readBack{};
    Check(packet.Read(id) && id == static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync),
          "the id leads the packet");
    Check(packet.ReadBytes(&readBack, sizeof(readBack)) && readBack.health == 100,
          "the payload follows unchanged");
}

void TestNothingIsWrittenWhenHeldBack() {
    samp::protocol::SyncThrottle throttle;
    samp::protocol::OnFootSyncData sync{};

    samp::net::BitStream first;
    Check(samp::protocol::BuildSyncPacket(throttle, samp::protocol::PacketId::PlayerSync, sync, 0,
                                          first),
          "the first payload is built");

    samp::net::BitStream second;
    Check(!samp::protocol::BuildSyncPacket(throttle, samp::protocol::PacketId::PlayerSync, sync, 1,
                                           second),
          "an unchanged payload produces no packet");
    Check(second.GetNumberOfBitsUsed() == 0, "the stream is left untouched");
}

void TestEachSyncKindKeepsItsOwnBaseline() {

    samp::protocol::SyncThrottle onFoot;
    samp::protocol::SyncThrottle aim;

    samp::protocol::OnFootSyncData player{};
    samp::protocol::AimSyncData aiming{};

    samp::net::BitStream playerPacket;
    samp::net::BitStream aimPacket;

    Check(samp::protocol::BuildSyncPacket(onFoot, samp::protocol::PacketId::PlayerSync, player, 0,
                                          playerPacket),
          "the player payload is built");
    Check(samp::protocol::BuildSyncPacket(aim, samp::protocol::PacketId::AimSync, aiming, 0,
                                          aimPacket),
          "the aim payload is built independently");

    Check(playerPacket.GetNumberOfBitsUsed() == (1 + 68) * 8, "player sync keeps its size");
    Check(aimPacket.GetNumberOfBitsUsed() == (1 + 31) * 8, "aim sync keeps its own size");
}

void TestSyncInterval() {
    using samp::protocol::SyncIntervalMs;

    Check(SyncIntervalMs(false, true, 30, 50) == samp::protocol::kIdleSyncIntervalMs,
          "with no player object the idle interval wins over everything else");

    Check(SyncIntervalMs(true, true, 30, 50) == samp::protocol::kHighRateSyncIntervalMs,
          "the high rate is fixed and ignores the player count");

    Check(SyncIntervalMs(true, false, 30, 0) == 30, "an empty area uses the base interval");
    Check(SyncIntervalMs(true, false, 30, 40) == 70,
          "each tracked player adds a millisecond to the interval");
}

}

int main() {
    TestSyncInterval();
    TestBuiltPacketShape();
    TestNothingIsWrittenWhenHeldBack();
    TestEachSyncKindKeepsItsOwnBaseline();
    TestFirstPayloadAlwaysGoes();
    TestUnchangedPayloadIsHeldBack();
    TestUnchangedPayloadGoesAfterTheInterval();
    TestAnyChangeGoesImmediately();
    TestSmallestPossibleChangeIsNoticed();
    TestClockWrapDoesNotStallSending();
    TestResetForgetsTheBaseline();

    if (g_failures == 0) {
        std::printf("All sync throttle tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
