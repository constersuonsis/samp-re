#pragma once

#include "samp/crypto/xtea.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace samp::crypto {

class PacketCodec {
public:

    static constexpr std::size_t kHeaderSize = 2;

    using RandomByteSource = std::function<std::uint8_t()>;

    PacketCodec(XteaCipher cipher, RandomByteSource randomByte);

    bool Encode(const void* payload, std::size_t size, std::vector<std::uint8_t>& packet) const;

    bool Decode(const void* packet, std::size_t size, std::vector<std::uint8_t>& payload) const;

    static std::size_t EncodedSize(std::size_t payloadSize);

private:
    XteaCipher cipher_;
    RandomByteSource randomByte_;
};

}
