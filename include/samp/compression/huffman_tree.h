#pragma once

#include "samp/network/bit_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace samp::compression {

/// Canonical byte-wise Huffman coder used for compressed text in the protocol.
///
/// The shape of the tree is part of the wire format: both ends build it from
/// the same frequency table and must agree bit for bit, so the construction
/// order here is deliberate rather than incidental.
class HuffmanTree {
public:
    static constexpr std::size_t kSymbolCount = 256;

    using FrequencyTable = std::array<std::uint32_t, kSymbolCount>;

    /// Builds the tree. Symbols with a frequency of zero are given a weight of
    /// one so that every byte value stays encodable.
    explicit HuffmanTree(const FrequencyTable& frequencies);

    /// Walks `bitCount` bits and writes the decoded bytes into `output`.
    ///
    /// Returns the number of symbols the bits decode to, which may exceed
    /// `capacity`: decoding always consumes the whole run so the stream stays
    /// aligned for whatever follows, and only the part that fits is written.
    std::size_t Decode(net::BitStream& stream, int bitCount, void* output,
                       std::size_t capacity) const;

    /// Appends the encoded form of `input` and returns how many bits it took.
    int Encode(const void* input, std::size_t size, net::BitStream& stream) const;

    /// Length in bits of a single symbol's code.
    std::size_t GetCodeLength(std::uint8_t symbol) const {
        return codes_[symbol].size();
    }

    /// The tree every connection starts with, built from the built-in text
    /// frequencies. Compressed strings that name no other tree use this one.
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

/// Default ceiling on how much a compressed block may claim to expand to.
inline constexpr std::size_t kMaxDecompressedBlockSize = 1u << 20;

/// Packs a block into a self-describing container.
///
/// Unlike compressed text, which relies on a tree both ends already agree on,
/// this form carries its own frequency table: the decoder rebuilds the exact
/// tree from what it reads rather than from anything shared beforehand. That
/// costs a header of 257 compressed words but makes the block stand alone.
bool CompressBlock(const void* input, std::size_t size, net::BitStream& out);

/// Unpacks a block written by CompressBlock.
///
/// The declared size arrives from the wire and is checked against `maxSize`
/// before anything is allocated, and the decoded length has to match what the
/// header promised.
bool DecompressBlock(net::BitStream& in, std::vector<std::uint8_t>& out,
                     std::size_t maxSize = kMaxDecompressedBlockSize);

}  // namespace samp::compression
