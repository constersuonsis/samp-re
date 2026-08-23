#pragma once

#include "samp/network/bit_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace samp::compression {

class HuffmanTree {
public:
    static constexpr std::size_t kSymbolCount = 256;

    using FrequencyTable = std::array<std::uint32_t, kSymbolCount>;

    explicit HuffmanTree(const FrequencyTable& frequencies);

    std::size_t Decode(net::BitStream& stream, int bitCount, void* output,
                       std::size_t capacity) const;

    int Encode(const void* input, std::size_t size, net::BitStream& stream) const;

    std::size_t GetCodeLength(std::uint8_t symbol) const {
        return codes_[symbol].size();
    }

    static const HuffmanTree& Default();

private:
    struct Node {
        std::uint8_t symbol = 0;
        std::uint32_t weight = 0;
        int left = kNoChild;
        int right = kNoChild;
        int parent = kNoChild;

        bool IsLeaf() const { return left == kNoChild && right == kNoChild; }
    };

    static constexpr int kNoChild = -1;

    void BuildCodeTable();

    std::vector<Node> nodes_;
    int root_ = kNoChild;
    std::array<std::vector<bool>, kSymbolCount> codes_;
};

inline constexpr std::size_t kMaxDecompressedBlockSize = 1u << 20;

bool CompressBlock(const void* input, std::size_t size, net::BitStream& out);

bool DecompressBlock(net::BitStream& in, std::vector<std::uint8_t>& out,
                     std::size_t maxSize = kMaxDecompressedBlockSize);

}
