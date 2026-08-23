#include "samp/protocol/packet_reader.h"

namespace samp::protocol {

bool ReadPacketHeader(net::BitStream& packet, PacketHeader& header) {
    header = PacketHeader{};

    std::uint8_t leading = 0;
    if (!packet.Read(leading)) {
        return false;
    }

    if (leading != static_cast<std::uint8_t>(PacketId::Timestamp)) {
        header.id = leading;
        return true;
    }

    if (!packet.Read(header.timestamp) || !packet.Read(header.id)) {
        header = PacketHeader{};
        return false;
    }

    header.hasTimestamp = true;
    return true;
}

void WritePacketHeader(net::BitStream& packet, const PacketHeader& header) {
    if (header.hasTimestamp) {
        packet.Write(static_cast<std::uint8_t>(PacketId::Timestamp));
        packet.Write(header.timestamp);
    }

    packet.Write(header.id);
}

}
