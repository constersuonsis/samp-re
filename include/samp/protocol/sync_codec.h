#pragma once

#include "samp/core/vector3.h"
#include "samp/network/bit_stream.h"
#include "samp/protocol/sync_structures.h"

#include <cstdint>

namespace samp::protocol {

// Vehicle sync travels in two different shapes. A client sends its own state as
// a plain block of bytes, and the server re-encodes it into the bit-packed form
// below before relaying it to everyone else. The client therefore only ever
// decodes this format; the matching writers are the exact inverse and exist so
// the codec can be tested and so tools can produce the relayed form.

/// Quantisation helpers for the health and armour nibbles.
///
/// Both values share a single byte, which costs all precision above 98: the
/// nibble stores the value in steps of seven, with the top code reserved for a
/// full 100.
std::uint8_t EncodeHealthNibble(std::uint8_t value);
std::uint8_t DecodeHealthNibble(std::uint8_t nibble);

/// A rotation is sent as three components plus four sign bits; the fourth
/// component is recovered from the unit-length constraint.
void WriteCompressedQuaternion(net::BitStream& stream, const Quaternion& rotation);
bool ReadCompressedQuaternion(net::BitStream& stream, Quaternion& rotation);

/// A velocity is sent as a full-precision magnitude plus a direction whose
/// components are quantised into 16 bits each. Vectors close to zero skip the
/// direction entirely.
void WriteCompressedVelocity(net::BitStream& stream, const Vector3& velocity);
bool ReadCompressedVelocity(net::BitStream& stream, Vector3& velocity);

/// Reads the body of a vehicle sync packet, positioned just past the packet id.
bool ReadVehicleSync(net::BitStream& stream, std::uint16_t& playerId, VehicleSyncData& sync);

/// Writes the body of a vehicle sync packet in the same order it is read.
void WriteVehicleSync(net::BitStream& stream, std::uint16_t playerId, const VehicleSyncData& sync);

}  // namespace samp::protocol
