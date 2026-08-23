#include "samp/crypto/rsa.h"

#include <cstring>

using std::memcmp;
using std::memcpy;
using std::memset;

#include "RSACrypt.h"

namespace samp::crypto {
namespace {

static_assert(sizeof(RSA_BIT_SIZE) == kRsaBlockSize,
              "the wrapper block size must match the vendored modulus width");

void ToBlock(const RSA_BIT_SIZE& value, RsaBlock& block) {
    std::memcpy(block.data(), value, kRsaBlockSize);
}

void FromBlock(const RsaBlock& block, RSA_BIT_SIZE& value) {
    std::memcpy(value, block.data(), kRsaBlockSize);
}

}

class RsaKeyPair::Impl {
public:
    big::RSACrypt<RSA_BIT_SIZE> crypt;
};

RsaKeyPair::RsaKeyPair() : impl_(std::make_unique<Impl>()) {}

RsaKeyPair::~RsaKeyPair() = default;

void RsaKeyPair::Generate() {
    impl_->crypt.generateKeys();
}

RsaPublicKey RsaKeyPair::PublicKey() const {
    RsaPublicKey key;
    RSA_BIT_SIZE modulus;
    impl_->crypt.getPublicKey(key.exponent, modulus);
    ToBlock(modulus, key.modulus);
    return key;
}

bool RsaKeyPair::Decrypt(const RsaBlock& ciphertext, RsaBlock& plaintext) const {
    RSA_BIT_SIZE cipher;
    RSA_BIT_SIZE plain;
    FromBlock(ciphertext, cipher);

    impl_->crypt.decrypt(cipher, plain);
    ToBlock(plain, plaintext);
    return true;
}

class RsaEncryptor::Impl {
public:
    big::RSACrypt<RSA_BIT_SIZE> crypt;
};

RsaEncryptor::RsaEncryptor(const RsaPublicKey& key) : impl_(std::make_unique<Impl>()) {
    RSA_BIT_SIZE modulus;
    FromBlock(key.modulus, modulus);
    impl_->crypt.setPublicKey(key.exponent, modulus);
}

RsaEncryptor::~RsaEncryptor() = default;

bool RsaEncryptor::Encrypt(const RsaBlock& plaintext, RsaBlock& ciphertext) const {
    RSA_BIT_SIZE plain;
    RSA_BIT_SIZE cipher;
    FromBlock(plaintext, plain);

    impl_->crypt.encrypt(plain, cipher);
    ToBlock(cipher, ciphertext);
    return true;
}

}
