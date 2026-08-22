#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace samp::protocol {

/// Decides whether a sync payload is worth putting on the wire.
///
/// Outgoing sync is not sent every frame. A payload goes out when it differs
/// from the one before it, or when enough time has passed that the server needs
/// reassuring that the client is still there. Both halves matter: dropping the
/// comparison floods the connection with identical packets, and dropping the
/// timer lets a motionless player go silent.
///
/// One of these belongs to each sync kind a client sends, since each has its
/// own last-sent payload.
class SyncThrottle {
public:
    /// How long an unchanged payload may go unsent.
    static constexpr std::uint32_t kResendIntervalMs = 500;

    /// Reports whether the payload should be sent, and records it if so.
    ///
    /// `nowMs` is a monotonic millisecond clock; the comparison is written to
    /// survive it wrapping.
    bool ShouldSend(const void* payload, std::size_t size, std::uint32_t nowMs);

    /// Forgets the last payload, so the next one is always sent.
    void Reset();

    bool HasSent() const { return hasSent_; }

private:
    std::vector<std::uint8_t> lastPayload_;
    std::uint32_t lastSentMs_ = 0;
    bool hasSent_ = false;
};

/// Interval used while there is no local player to report on.
inline constexpr std::uint32_t kIdleSyncIntervalMs = 1000;

/// Fixed interval used when the server asked for the high rate.
inline constexpr std::uint32_t kHighRateSyncIntervalMs = 15;

/// How long to wait between outgoing sync packets.
///
/// Without a local player object there is nothing to report, so the client
/// falls back to a slow heartbeat. Otherwise the server either pins the rate to
/// a fixed high value, or lets it scale: the base interval it sent at connect
/// time plus one millisecond per player currently being tracked, so a crowded
/// area costs every client a little more time between updates.
constexpr std::uint32_t SyncIntervalMs(bool hasPlayerObject, bool highRate,
                                       std::uint32_t baseIntervalMs,
                                       std::uint8_t trackedPlayers) {
    if (!hasPlayerObject) {
        return kIdleSyncIntervalMs;
    }
    if (highRate) {
        return kHighRateSyncIntervalMs;
    }
    return baseIntervalMs + trackedPlayers;
}

}  // namespace samp::protocol
