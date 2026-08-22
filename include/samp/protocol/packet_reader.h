#pragma once

#include "samp/network/bit_stream.h"
#include "samp/protocol/sync_structures.h"

#include <cstdint>

namespace samp::protocol {

/// The leading bytes of a packet, once any timestamp has been stripped.
struct PacketHeader {
    /// The id of the message that follows.
    std::uint8_t id = 0;

    /// Send time, when the packet carried one.
    std::uint32_t timestamp = 0;
    bool hasTimestamp = false;
};

/// Reads a packet's header and leaves the stream on the payload.
///
/// A packet may be prefixed with a timestamp marker, in which case the real id
/// comes after the send time rather than first. Treating the marker as the id
/// routes the packet to the wrong handler and leaves four bytes of time in
/// front of the payload, so the prefix has to be resolved before anything else
/// looks at the packet.
bool ReadPacketHeader(net::BitStream& packet, PacketHeader& header);

/// Writes an id, optionally behind a timestamp prefix.
void WritePacketHeader(net::BitStream& packet, const PacketHeader& header);

}  // namespace samp::protocol
