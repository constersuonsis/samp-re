#pragma once

#include "samp/core/vector3.h"
#include "samp/network/bit_stream.h"
#include "samp/protocol/sync_structures.h"

#include <cstdint>

namespace samp::protocol {

std::uint8_t EncodeHealthNibble(std::uint8_t value);
std::uint8_t DecodeHealthNibble(std::uint8_t nibble);

void WriteCompressedQuaternion(net::BitStream& stream, const Quaternion& rotation);
bool ReadCompressedQuaternion(net::BitStream& stream, Quaternion& rotation);

void WriteCompressedVelocity(net::BitStream& stream, const Vector3& velocity);
bool ReadCompressedVelocity(net::BitStream& stream, Vector3& velocity);

bool ReadVehicleSync(net::BitStream& stream, std::uint16_t& playerId, VehicleSyncData& sync);

void WriteVehicleSync(net::BitStream& stream, std::uint16_t playerId, const VehicleSyncData& sync);

bool ReadPlayerSync(net::BitStream& stream, std::uint16_t& playerId, OnFootSyncData& sync);

void WritePlayerSync(net::BitStream& stream, std::uint16_t playerId, const OnFootSyncData& sync);

bool ReadRelayedSync(net::BitStream& stream, std::uint16_t& playerId, void* data,
                     std::size_t size);

void WriteRelayedSync(net::BitStream& stream, std::uint16_t playerId, const void* data,
                      std::size_t size);

}
