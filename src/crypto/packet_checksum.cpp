#include "samp/crypto/packet_checksum.h"

namespace samp::crypto {

std::uint8_t PacketChecksum::UpdateByte(std::uint8_t value) {
    const auto mixed = static_cast<std::uint8_t>(value ^ (key_ >> 8));
    key_ = static_cast<std::uint16_t>((key_ + mixed) * kMultiplier + kAddend);
    total_ += mixed;
    return mixed;
}

void PacketChecksum::Update(const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        UpdateByte(bytes[i]);
    }
}

std::uint8_t PacketChecksum::Value() const {
    return static_cast<std::uint8_t>(total_ ^ (total_ << 4));
}

void PacketChecksum::Reset() {
    key_ = kInitialKey;
    total_ = 0;
}

}
