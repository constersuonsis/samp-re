#pragma once

#include "samp/network/bit_stream.h"
#include "samp/protocol/sync_structures.h"

#include <cstdint>

namespace samp::protocol {

struct PacketHeader {

    std::uint8_t id = 0;

    std::uint32_t timestamp = 0;
    bool hasTimestamp = false;
};

bool ReadPacketHeader(net::BitStream& packet, PacketHeader& header);

void WritePacketHeader(net::BitStream& packet, const PacketHeader& header);

}
