#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace samp::protocol {

class SyncThrottle {
public:

    static constexpr std::uint32_t kResendIntervalMs = 500;

    bool ShouldSend(const void* payload, std::size_t size, std::uint32_t nowMs);

    void Reset();

    bool HasSent() const { return hasSent_; }

private:
    std::vector<std::uint8_t> lastPayload_;
    std::uint32_t lastSentMs_ = 0;
    bool hasSent_ = false;
};

inline constexpr std::uint32_t kIdleSyncIntervalMs = 1000;

inline constexpr std::uint32_t kHighRateSyncIntervalMs = 15;

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

}
