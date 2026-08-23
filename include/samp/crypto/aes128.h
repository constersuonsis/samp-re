#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace samp::crypto {

class Aes128 {
public:
    static constexpr std::size_t kKeySize = 16;
    static constexpr std::size_t kBlockSize = 16;
    static constexpr int kRounds = 10;

    using Key = std::array<std::uint8_t, kKeySize>;
    using Block = std::array<std::uint8_t, kBlockSize>;

    explicit Aes128(const Key& key);

    void EncryptBlock(const Block& input, Block& output) const;
    void DecryptBlock(const Block& input, Block& output) const;

private:
    std::array<std::uint32_t, 4 * (kRounds + 1)> roundKeys_;
};

bool EncryptCbc(const Aes128& cipher, const Aes128::Block& iv, const std::vector<std::uint8_t>& plaintext,
                std::vector<std::uint8_t>& ciphertext);

bool DecryptCbc(const Aes128& cipher, const Aes128::Block& iv, const std::vector<std::uint8_t>& ciphertext,
                std::vector<std::uint8_t>& plaintext);

void ApplyCtr(const Aes128& cipher, const Aes128::Block& nonce, const std::vector<std::uint8_t>& input,
              std::vector<std::uint8_t>& output);

}
