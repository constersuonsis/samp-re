#include "samp/crypto/xtea.h"

#include <cstring>

namespace samp::crypto {
namespace {

/// The mixing step both halves go through before the round key is applied.
std::uint32_t Mix(std::uint32_t value) {
    return value + ((value << 4) ^ (value >> 5));
}

}  // namespace

XteaCipher::XteaCipher(const Key& key, std::uint32_t delta) : key_(key), delta_(delta) {}

void XteaCipher::EncryptBlock(std::uint32_t& left, std::uint32_t& right) const {
    std::uint32_t sum = 0;

    for (int round = 0; round < kRounds; ++round) {
        left += Mix(right) ^ (sum + key_[sum & 3]);
        sum += delta_;
        right += Mix(left) ^ (sum + key_[(sum >> 11) & 3]);
    }
}

void XteaCipher::DecryptBlock(std::uint32_t& left, std::uint32_t& right) const {
    std::uint32_t sum = delta_ * kRounds;

    for (int round = 0; round < kRounds; ++round) {
        right -= Mix(left) ^ (sum + key_[(sum >> 11) & 3]);
        sum -= delta_;
        left -= Mix(right) ^ (sum + key_[sum & 3]);
    }
}

bool XteaCipher::Encrypt(void* data, std::size_t size) const {
    if (size % kBlockSize != 0) {
        return false;
    }

    auto* bytes = static_cast<std::uint8_t*>(data);
    for (std::size_t offset = 0; offset < size; offset += kBlockSize) {
        std::uint32_t left = 0;
        std::uint32_t right = 0;
        std::memcpy(&left, bytes + offset, sizeof(left));
        std::memcpy(&right, bytes + offset + sizeof(left), sizeof(right));

        EncryptBlock(left, right);

        std::memcpy(bytes + offset, &left, sizeof(left));
        std::memcpy(bytes + offset + sizeof(left), &right, sizeof(right));
    }
    return true;
}

bool XteaCipher::Decrypt(void* data, std::size_t size) const {
    if (size % kBlockSize != 0) {
        return false;
    }

    auto* bytes = static_cast<std::uint8_t*>(data);
    for (std::size_t offset = 0; offset < size; offset += kBlockSize) {
        std::uint32_t left = 0;
        std::uint32_t right = 0;
        std::memcpy(&left, bytes + offset, sizeof(left));
        std::memcpy(&right, bytes + offset + sizeof(left), sizeof(right));

        DecryptBlock(left, right);

        std::memcpy(bytes + offset, &left, sizeof(left));
        std::memcpy(bytes + offset + sizeof(left), &right, sizeof(right));
    }
    return true;
}

}  // namespace samp::crypto
