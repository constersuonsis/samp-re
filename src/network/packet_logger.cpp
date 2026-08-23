#include "samp/network/packet_logger.h"

#include <cstdio>

namespace samp::net {

namespace {

std::string DescribeMessage(const char* kind, unsigned id, std::size_t bytes) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s id=%u bytes=%zu", kind, id, bytes);
    return buffer;
}

}

PacketLogger::PacketLogger(ConnectionHandler& downstream, LogSink sink)
    : downstream_(downstream), sink_(std::move(sink)) {}

void PacketLogger::SetEnabled(bool enabled) {
    enabled_ = enabled;
}

void PacketLogger::Write(const std::string& line) {
    if (!enabled_ || !sink_) {
        return;
    }
    sink_(line);
}

void PacketLogger::OnPacket(const BitStream& packet) {
    Write(DescribeMessage("packet", packet.GetNumberOfBytesUsed() > 0 ? packet.GetData()[0] : 0,
                          packet.GetNumberOfBytesUsed()));
    downstream_.OnPacket(packet);
}

void PacketLogger::OnRpc(std::uint8_t id, const BitStream& payload) {
    Write(DescribeMessage("rpc", id, payload.GetNumberOfBytesUsed()));
    downstream_.OnRpc(id, payload);
}

void PacketLogger::OnConnected() {
    Write("connected");
    downstream_.OnConnected();
}

void PacketLogger::OnDisconnected() {
    Write("disconnected");
    downstream_.OnDisconnected();
}

}
