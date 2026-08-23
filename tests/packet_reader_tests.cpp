#include "samp/protocol/packet_reader.h"

#include "samp/protocol/sync_codec.h"

#include <cstdio>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

void TestPlainHeader() {
    samp::net::BitStream packet;
    samp::protocol::WritePacketHeader(
        packet, {static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync), 0, false});
    Check(packet.GetNumberOfBitsUsed() == 8, "a plain header is one byte");

    samp::protocol::PacketHeader header;
    Check(samp::protocol::ReadPacketHeader(packet, header), "a plain header reads");
    Check(header.id == static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync),
          "the id round-trips");
    Check(!header.hasTimestamp, "no timestamp is reported when none was sent");
}

void TestTimestampedHeader() {
    samp::net::BitStream packet;
    samp::protocol::WritePacketHeader(
        packet, {static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync), 123456, true});
    Check(packet.GetNumberOfBitsUsed() == 6 * 8, "a timestamped header is six bytes");

    samp::protocol::PacketHeader header;
    Check(samp::protocol::ReadPacketHeader(packet, header), "a timestamped header reads");
    Check(header.hasTimestamp && header.timestamp == 123456, "the send time round-trips");
    Check(header.id == static_cast<std::uint8_t>(samp::protocol::PacketId::PlayerSync),
          "the real id is recovered from behind the marker");
}

void TestPayloadStartsWhereExpected() {

    samp::protocol::VehicleSyncData sent;
    sent.vehicleId = 77;
    sent.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    sent.position = {1.0f, 2.0f, 3.0f};
    sent.vehicleHealth = 1000.0f;
    sent.playerHealth = 100;

    samp::net::BitStream packet;
    samp::protocol::WritePacketHeader(packet, {0xC8, 999, true});
    samp::protocol::WriteVehicleSync(packet, 4, sent);

    samp::protocol::PacketHeader header;
    Check(samp::protocol::ReadPacketHeader(packet, header), "the header reads");
    Check(header.id == 0xC8 && header.timestamp == 999, "header fields round-trip");

    std::uint16_t playerId = 0;
    samp::protocol::VehicleSyncData received;
    Check(samp::protocol::ReadVehicleSync(packet, playerId, received),
          "the payload parses straight after the header");
    Check(playerId == 4 && received.vehicleId == 77, "the payload is intact");
}

void TestMarkerIsNotMistakenForAnId() {

    samp::net::BitStream packet;
    packet.Write(static_cast<std::uint8_t>(samp::protocol::PacketId::Timestamp));
    packet.Write<std::uint32_t>(42);
    packet.Write<std::uint8_t>(0xCF);

    samp::protocol::PacketHeader header;
    Check(samp::protocol::ReadPacketHeader(packet, header), "the header reads");
    Check(header.id != static_cast<std::uint8_t>(samp::protocol::PacketId::Timestamp),
          "the marker is consumed rather than reported as the id");
    Check(header.id == 0xCF, "the id behind the marker is the one reported");
}

void TestTruncatedTimestampIsRejected() {
    samp::net::BitStream packet;
    packet.Write(static_cast<std::uint8_t>(samp::protocol::PacketId::Timestamp));
    packet.Write<std::uint16_t>(1);

    samp::protocol::PacketHeader header;
    Check(!samp::protocol::ReadPacketHeader(packet, header),
          "a marker with no room for the time and id behind it is rejected");
    Check(header.id == 0, "a rejected header leaves nothing behind");
}

}

int main() {
    TestPlainHeader();
    TestTimestampedHeader();
    TestPayloadStartsWhereExpected();
    TestMarkerIsNotMistakenForAnId();
    TestTruncatedTimestampIsRejected();

    if (g_failures == 0) {
        std::printf("All packet reader tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
