#pragma once

#include <cstddef>
#include <cstdint>

namespace samp::crypto {

/// Rolling checksum guarding the encrypted packet body.
///
/// Each byte is folded against the high half of a key that then advances
/// through a linear congruential step. The three constants below are part of
/// the wire format and are what a receiver reproduces to validate a packet.
class PacketChecksum {
public:
    static constexpr std::uint16_t kInitialKey = 55665;
    static constexpr std::uint16_t kMultiplier = 52845;
    static constexpr std::uint16_t kAddend = 22719;

    void Update(const void* data, std::size_t size);

    /// The byte that travels with the packet: the accumulated total folded onto
    /// itself so that both halves of it contribute.
    std::uint8_t Value() const;

    void Reset();

private:
    std::uint8_t UpdateByte(std::uint8_t value);

    std::uint16_t key_ = kInitialKey;
    std::uint32_t total_ = 0;
};

}  // namespace samp::crypto
