#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::crypto {

/// The block cipher applied to outgoing packets.
///
/// It is XTEA over 64-bit blocks with the usual thirty-two rounds, but the key
/// schedule differs from the textbook version: the round key is picked out of a
/// four-*byte* array rather than four 32-bit words, so the effective key is
/// only 32 bits wide.
class XteaCipher {
public:
    static constexpr std::size_t kKeySize = 4;
    static constexpr std::size_t kBlockSize = 8;
    static constexpr int kRounds = 32;

    using Key = std::array<std::uint8_t, kKeySize>;

    /// `delta` is the per-round constant. It is not fixed in the client: it is
    /// held alongside the key and set up with the connection, so it has to be
    /// supplied rather than assumed.
    XteaCipher(const Key& key, std::uint32_t delta);

    void EncryptBlock(std::uint32_t& left, std::uint32_t& right) const;
    void DecryptBlock(std::uint32_t& left, std::uint32_t& right) const;

    /// Transforms a buffer in place. The size must be a multiple of the block
    /// size; anything else is left untouched and reported as a failure.
    bool Encrypt(void* data, std::size_t size) const;
    bool Decrypt(void* data, std::size_t size) const;

private:
    Key key_;
    std::uint32_t delta_;
};

}  // namespace samp::crypto
