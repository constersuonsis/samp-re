#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::crypto {

class XteaCipher {
public:
    static constexpr std::size_t kKeySize = 4;
    static constexpr std::size_t kBlockSize = 8;
    static constexpr int kRounds = 32;

    using Key = std::array<std::uint8_t, kKeySize>;

    XteaCipher(const Key& key, std::uint32_t delta);

    void EncryptBlock(std::uint32_t& left, std::uint32_t& right) const;
    void DecryptBlock(std::uint32_t& left, std::uint32_t& right) const;

    bool Encrypt(void* data, std::size_t size) const;
    bool Decrypt(void* data, std::size_t size) const;

private:
    Key key_;
    std::uint32_t delta_;
};

}
