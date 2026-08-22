#pragma once

#include "samp/crypto/xtea.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace samp::crypto {

/// Frames a payload the way the client puts it on the wire.
///
/// A packet is a checksum byte, a byte holding the padding length, that many
/// bytes of filler, and then the payload. The whole thing is padded to a
/// multiple of the cipher's block size and encrypted.
class PacketCodec {
public:
    /// Checksum byte plus the byte describing the padding.
    static constexpr std::size_t kHeaderSize = 2;

    /// Supplies the filler bytes and the random half of the padding byte.
    /// Injectable so the framing can be exercised deterministically.
    using RandomByteSource = std::function<std::uint8_t()>;

    PacketCodec(XteaCipher cipher, RandomByteSource randomByte);

    bool Encode(const void* payload, std::size_t size, std::vector<std::uint8_t>& packet) const;

    /// Reverses Encode and checks the checksum. A packet whose length is not a
    /// whole number of blocks, or whose checksum disagrees, is rejected.
    bool Decode(const void* packet, std::size_t size, std::vector<std::uint8_t>& payload) const;

    /// Size the given payload will occupy once framed and padded.
    static std::size_t EncodedSize(std::size_t payloadSize);

private:
    XteaCipher cipher_;
    RandomByteSource randomByte_;
};

}  // namespace samp::crypto
