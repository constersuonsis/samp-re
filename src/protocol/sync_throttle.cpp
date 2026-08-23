#include "samp/protocol/sync_throttle.h"

#include <cstring>

namespace samp::protocol {

bool SyncThrottle::ShouldSend(const void* payload, std::size_t size, std::uint32_t nowMs) {
    const auto* bytes = static_cast<const std::uint8_t*>(payload);

    const bool overdue = !hasSent_ || (nowMs - lastSentMs_) > kResendIntervalMs;
    const bool changed = lastPayload_.size() != size ||
                         (size > 0 && std::memcmp(lastPayload_.data(), bytes, size) != 0);

    if (!overdue && !changed) {
        return false;
    }

    lastPayload_.assign(bytes, bytes + size);
    lastSentMs_ = nowMs;
    hasSent_ = true;
    return true;
}

void SyncThrottle::Reset() {
    lastPayload_.clear();
    lastSentMs_ = 0;
    hasSent_ = false;
}

}
