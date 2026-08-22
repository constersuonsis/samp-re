#pragma once

#include "samp/network/bit_stream.h"
#include "samp/protocol/sync_structures.h"
#include "samp/protocol/sync_throttle.h"

#include <cstdint>
#include <type_traits>

namespace samp::protocol {

/// Builds an outgoing sync packet, if one is due.
///
/// Every outgoing sync kind shares the same shape: a single id byte followed by
/// the payload copied out verbatim. Nothing is quantised or bit-packed on the
/// way out — that only happens when the server relays the data on — so one
/// builder serves all of them.
///
/// Returns false when the throttle decided the payload was not worth sending,
/// in which case `packet` is left untouched.
template <typename Sync>
bool BuildSyncPacket(SyncThrottle& throttle, PacketId id, const Sync& sync, std::uint32_t nowMs,
                     net::BitStream& packet) {
    static_assert(std::is_trivially_copyable_v<Sync>,
                  "sync payloads are copied out as a block of bytes");

    if (!throttle.ShouldSend(&sync, sizeof(Sync), nowMs)) {
        return false;
    }

    packet.Write(static_cast<std::uint8_t>(id));
    packet.WriteBytes(&sync, sizeof(Sync));
    return true;
}

}  // namespace samp::protocol
