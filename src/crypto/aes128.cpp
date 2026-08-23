#include "samp/crypto/aes128.h"

namespace samp::crypto {
namespace {

constexpr std::array<std::uint8_t, 256> kSBox = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

constexpr std::array<std::uint8_t, 256> BuildInverseSBox() {
    std::array<std::uint8_t, 256> inverse{};
    for (int i = 0; i < 256; ++i) {
        inverse[kSBox[static_cast<std::size_t>(i)]] = static_cast<std::uint8_t>(i);
    }
    return inverse;
}

constexpr std::array<std::uint8_t, 256> kInverseSBox = BuildInverseSBox();

constexpr std::array<std::uint8_t, 11> kRcon = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
};

std::uint8_t GfMultiply(std::uint8_t a, std::uint8_t b) {
    std::uint8_t result = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
            result ^= a;
        }
        const bool highBitSet = (a & 0x80) != 0;
        a = static_cast<std::uint8_t>(a << 1);
        if (highBitSet) {
            a ^= 0x1b;
        }
        b = static_cast<std::uint8_t>(b >> 1);
    }
    return result;
}

std::uint32_t SubWord(std::uint32_t word) {
    return (static_cast<std::uint32_t>(kSBox[(word >> 24) & 0xFF]) << 24) |
           (static_cast<std::uint32_t>(kSBox[(word >> 16) & 0xFF]) << 16) |
           (static_cast<std::uint32_t>(kSBox[(word >> 8) & 0xFF]) << 8) |
           static_cast<std::uint32_t>(kSBox[word & 0xFF]);
}

std::uint32_t RotWord(std::uint32_t word) {
    return (word << 8) | (word >> 24);
}

using State = std::array<std::array<std::uint8_t, 4>, 4>;

State ToState(const Aes128::Block& block) {
    State state{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            state[row][column] = block[static_cast<std::size_t>(column) * 4 + row];
        }
    }
    return state;
}

Aes128::Block FromState(const State& state) {
    Aes128::Block block{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            block[static_cast<std::size_t>(column) * 4 + row] = state[row][column];
        }
    }
    return block;
}

void AddRoundKey(State& state, const std::uint32_t* roundKey) {
    for (int column = 0; column < 4; ++column) {
        const std::uint32_t word = roundKey[column];
        state[0][column] ^= static_cast<std::uint8_t>(word >> 24);
        state[1][column] ^= static_cast<std::uint8_t>(word >> 16);
        state[2][column] ^= static_cast<std::uint8_t>(word >> 8);
        state[3][column] ^= static_cast<std::uint8_t>(word);
    }
}

void SubBytes(State& state) {
    for (auto& row : state) {
        for (auto& byte : row) {
            byte = kSBox[byte];
        }
    }
}

void InverseSubBytes(State& state) {
    for (auto& row : state) {
        for (auto& byte : row) {
            byte = kInverseSBox[byte];
        }
    }
}

void ShiftRows(State& state) {
    for (int row = 1; row < 4; ++row) {
        std::array<std::uint8_t, 4> shifted{};
        for (int column = 0; column < 4; ++column) {
            shifted[static_cast<std::size_t>(column)] =
                state[static_cast<std::size_t>(row)]
                     [static_cast<std::size_t>((column + row) % 4)];
        }
        state[static_cast<std::size_t>(row)] = shifted;
    }
}

void InverseShiftRows(State& state) {
    for (int row = 1; row < 4; ++row) {
        std::array<std::uint8_t, 4> shifted{};
        for (int column = 0; column < 4; ++column) {
            shifted[static_cast<std::size_t>((column + row) % 4)] =
                state[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        }
        state[static_cast<std::size_t>(row)] = shifted;
    }
}

void MixColumns(State& state) {
    for (int column = 0; column < 4; ++column) {
        const std::uint8_t a0 = state[0][column];
        const std::uint8_t a1 = state[1][column];
        const std::uint8_t a2 = state[2][column];
        const std::uint8_t a3 = state[3][column];

        state[0][column] = static_cast<std::uint8_t>(GfMultiply(a0, 2) ^ GfMultiply(a1, 3) ^ a2 ^ a3);
        state[1][column] = static_cast<std::uint8_t>(a0 ^ GfMultiply(a1, 2) ^ GfMultiply(a2, 3) ^ a3);
        state[2][column] = static_cast<std::uint8_t>(a0 ^ a1 ^ GfMultiply(a2, 2) ^ GfMultiply(a3, 3));
        state[3][column] = static_cast<std::uint8_t>(GfMultiply(a0, 3) ^ a1 ^ a2 ^ GfMultiply(a3, 2));
    }
}

void InverseMixColumns(State& state) {
    for (int column = 0; column < 4; ++column) {
        const std::uint8_t a0 = state[0][column];
        const std::uint8_t a1 = state[1][column];
        const std::uint8_t a2 = state[2][column];
        const std::uint8_t a3 = state[3][column];

        state[0][column] = static_cast<std::uint8_t>(GfMultiply(a0, 14) ^ GfMultiply(a1, 11) ^
                                                      GfMultiply(a2, 13) ^ GfMultiply(a3, 9));
        state[1][column] = static_cast<std::uint8_t>(GfMultiply(a0, 9) ^ GfMultiply(a1, 14) ^
                                                      GfMultiply(a2, 11) ^ GfMultiply(a3, 13));
        state[2][column] = static_cast<std::uint8_t>(GfMultiply(a0, 13) ^ GfMultiply(a1, 9) ^
                                                      GfMultiply(a2, 14) ^ GfMultiply(a3, 11));
        state[3][column] = static_cast<std::uint8_t>(GfMultiply(a0, 11) ^ GfMultiply(a1, 13) ^
                                                      GfMultiply(a2, 9) ^ GfMultiply(a3, 14));
    }
}

}

Aes128::Aes128(const Key& key) {
    constexpr int kWordsInKey = 4;

    for (int i = 0; i < kWordsInKey; ++i) {
        roundKeys_[static_cast<std::size_t>(i)] =
            (static_cast<std::uint32_t>(key[static_cast<std::size_t>(i) * 4]) << 24) |
            (static_cast<std::uint32_t>(key[static_cast<std::size_t>(i) * 4 + 1]) << 16) |
            (static_cast<std::uint32_t>(key[static_cast<std::size_t>(i) * 4 + 2]) << 8) |
            static_cast<std::uint32_t>(key[static_cast<std::size_t>(i) * 4 + 3]);
    }

    const int totalWords = 4 * (kRounds + 1);
    for (int i = kWordsInKey; i < totalWords; ++i) {
        std::uint32_t temp = roundKeys_[static_cast<std::size_t>(i - 1)];
        if (i % kWordsInKey == 0) {
            temp = SubWord(RotWord(temp)) ^
                   (static_cast<std::uint32_t>(kRcon[static_cast<std::size_t>(i / kWordsInKey)]) << 24);
        }
        roundKeys_[static_cast<std::size_t>(i)] =
            roundKeys_[static_cast<std::size_t>(i - kWordsInKey)] ^ temp;
    }
}

void Aes128::EncryptBlock(const Block& input, Block& output) const {
    State state = ToState(input);

    AddRoundKey(state, &roundKeys_[0]);

    for (int round = 1; round < kRounds; ++round) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, &roundKeys_[static_cast<std::size_t>(round) * 4]);
    }

    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, &roundKeys_[static_cast<std::size_t>(kRounds) * 4]);

    output = FromState(state);
}

void Aes128::DecryptBlock(const Block& input, Block& output) const {
    State state = ToState(input);

    AddRoundKey(state, &roundKeys_[static_cast<std::size_t>(kRounds) * 4]);

    for (int round = kRounds - 1; round > 0; --round) {
        InverseShiftRows(state);
        InverseSubBytes(state);
        AddRoundKey(state, &roundKeys_[static_cast<std::size_t>(round) * 4]);
        InverseMixColumns(state);
    }

    InverseShiftRows(state);
    InverseSubBytes(state);
    AddRoundKey(state, &roundKeys_[0]);

    output = FromState(state);
}

bool EncryptCbc(const Aes128& cipher, const Aes128::Block& iv,
                const std::vector<std::uint8_t>& plaintext,
                std::vector<std::uint8_t>& ciphertext) {
    if (plaintext.size() % Aes128::kBlockSize != 0) {
        ciphertext.clear();
        return false;
    }

    ciphertext.resize(plaintext.size());
    Aes128::Block chain = iv;

    for (std::size_t offset = 0; offset < plaintext.size(); offset += Aes128::kBlockSize) {
        Aes128::Block block{};
        for (std::size_t i = 0; i < Aes128::kBlockSize; ++i) {
            block[i] = static_cast<std::uint8_t>(plaintext[offset + i] ^ chain[i]);
        }

        Aes128::Block encrypted{};
        cipher.EncryptBlock(block, encrypted);

        for (std::size_t i = 0; i < Aes128::kBlockSize; ++i) {
            ciphertext[offset + i] = encrypted[i];
        }
        chain = encrypted;
    }

    return true;
}

bool DecryptCbc(const Aes128& cipher, const Aes128::Block& iv,
                const std::vector<std::uint8_t>& ciphertext,
                std::vector<std::uint8_t>& plaintext) {
    if (ciphertext.size() % Aes128::kBlockSize != 0) {
        plaintext.clear();
        return false;
    }

    plaintext.resize(ciphertext.size());
    Aes128::Block chain = iv;

    for (std::size_t offset = 0; offset < ciphertext.size(); offset += Aes128::kBlockSize) {
        Aes128::Block block{};
        for (std::size_t i = 0; i < Aes128::kBlockSize; ++i) {
            block[i] = ciphertext[offset + i];
        }

        Aes128::Block decrypted{};
        cipher.DecryptBlock(block, decrypted);

        for (std::size_t i = 0; i < Aes128::kBlockSize; ++i) {
            plaintext[offset + i] = static_cast<std::uint8_t>(decrypted[i] ^ chain[i]);
        }
        chain = block;
    }

    return true;
}

void ApplyCtr(const Aes128& cipher, const Aes128::Block& nonce,
              const std::vector<std::uint8_t>& input, std::vector<std::uint8_t>& output) {
    output.resize(input.size());

    Aes128::Block counter = nonce;

    for (std::size_t offset = 0; offset < input.size(); offset += Aes128::kBlockSize) {
        Aes128::Block keyStream{};
        cipher.EncryptBlock(counter, keyStream);

        const std::size_t remaining = input.size() - offset;
        const std::size_t chunk = remaining < Aes128::kBlockSize ? remaining : Aes128::kBlockSize;
        for (std::size_t i = 0; i < chunk; ++i) {
            output[offset + i] = static_cast<std::uint8_t>(input[offset + i] ^ keyStream[i]);
        }

        for (std::size_t i = Aes128::kBlockSize; i-- > 0;) {
            if (++counter[i] != 0) {
                break;
            }
        }
    }
}

}
