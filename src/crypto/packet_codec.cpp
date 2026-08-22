#include "samp/crypto/packet_codec.h"

#include "samp/crypto/packet_checksum.h"

#include <cstring>
#include <utility>

namespace samp::crypto {
namespace {

/// Low nibble of the padding byte; the high nibble is filler.
constexpr std::uint8_t kPaddingMask = 0x0F;

std::size_t PaddingFor(std::size_t framedSize) {
    const std::size_t remainder = framedSize % XteaCipher::kBlockSize;
    return remainder == 0 ? 0 : XteaCipher::kBlockSize - remainder;
}

}  // namespace

PacketCodec::PacketCodec(XteaCipher cipher, RandomByteSource randomByte)
    : cipher_(std::move(cipher)), randomByte_(std::move(randomByte)) {}

std::size_t PacketCodec::EncodedSize(std::size_t payloadSize) {
    const std::size_t framed = payloadSize + kHeaderSize;
    return framed + PaddingFor(framed);
}

bool PacketCodec::Encode(const void* payload, std::size_t size,
                         std::vector<std::uint8_t>& packet) const {
    const std::size_t framed = size + kHeaderSize;
    const std::size_t padding = PaddingFor(framed);

    packet.assign(framed + padding, 0);

    // The padding length shares a byte with filler so that a short packet does
    // not advertise its shape in the clear.
    packet[1] = static_cast<std::uint8_t>(padding | (randomByte_() << 4));
    for (std::size_t i = 0; i < padding; ++i) {
        packet[kHeaderSize + i] = randomByte_();
    }

    if (size > 0) {
        std::memcpy(packet.data() + kHeaderSize + padding, payload, size);
    }

    // Everything except the checksum byte itself is covered.
    PacketChecksum checksum;
    checksum.Update(packet.data() + 1, packet.size() - 1);
    packet[0] = checksum.Value();

    return cipher_.Encrypt(packet.data(), packet.size());
}

bool PacketCodec::Decode(const void* packet, std::size_t size,
                         std::vector<std::uint8_t>& payload) const {
    payload.clear();

    if (size < kHeaderSize || size % XteaCipher::kBlockSize != 0) {
        return false;
    }

    std::vector<std::uint8_t> plain(size);
    std::memcpy(plain.data(), packet, size);
    if (!cipher_.Decrypt(plain.data(), plain.size())) {
        return false;
    }

    PacketChecksum checksum;
    checksum.Update(plain.data() + 1, plain.size() - 1);
    if (checksum.Value() != plain[0]) {
        return false;
    }

    const std::size_t padding = plain[1] & kPaddingMask;
    if (kHeaderSize + padding > plain.size()) {
        return false;
    }

    payload.assign(plain.begin() + static_cast<std::ptrdiff_t>(kHeaderSize + padding), plain.end());
    return true;
}

}  // namespace samp::crypto
