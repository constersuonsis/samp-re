#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace samp::crypto {

inline constexpr std::size_t kRsaModulusBits = 256;
inline constexpr std::size_t kRsaBlockSize = kRsaModulusBits / 8;

using RsaBlock = std::array<std::uint8_t, kRsaBlockSize>;

struct RsaPublicKey {
    std::uint32_t exponent = 0;
    RsaBlock modulus{};
};

class RsaKeyPair {
public:
    RsaKeyPair();
    ~RsaKeyPair();

    RsaKeyPair(const RsaKeyPair&) = delete;
    RsaKeyPair& operator=(const RsaKeyPair&) = delete;

    void Generate();

    RsaPublicKey PublicKey() const;

    bool Decrypt(const RsaBlock& ciphertext, RsaBlock& plaintext) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class RsaEncryptor {
public:
    explicit RsaEncryptor(const RsaPublicKey& key);
    ~RsaEncryptor();

    RsaEncryptor(const RsaEncryptor&) = delete;
    RsaEncryptor& operator=(const RsaEncryptor&) = delete;

    bool Encrypt(const RsaBlock& plaintext, RsaBlock& ciphertext) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
