#include "samp/compression/huffman_tree.h"

#include "samp/compression/text_frequencies.h"

#include <algorithm>
#include <list>

namespace samp::compression {

HuffmanTree::HuffmanTree(const FrequencyTable& frequencies) {
    // Every merge adds one node, so the final tree holds 2 * 256 - 1 of them.
    nodes_.reserve(kSymbolCount * 2 - 1);

    // The queue stays sorted by ascending weight. Equal weights are ordered so
    // that the newest entry comes first, which is what decides the shape of the
    // tree whenever weights tie.
    std::list<int> queue;

    const auto insert = [&](int index) {
        const std::uint32_t weight = nodes_[index].weight;
        const auto position = std::find_if(queue.begin(), queue.end(), [&](int candidate) {
            return nodes_[candidate].weight >= weight;
        });
        queue.insert(position, index);
    };

    for (std::size_t symbol = 0; symbol < kSymbolCount; ++symbol) {
        Node leaf;
        leaf.symbol = static_cast<std::uint8_t>(symbol);
        leaf.weight = std::max<std::uint32_t>(frequencies[symbol], 1);

        nodes_.push_back(leaf);
        insert(static_cast<int>(nodes_.size()) - 1);
    }

    while (true) {
        const int left = queue.front();
        queue.pop_front();
        const int right = queue.front();
        queue.pop_front();

        Node branch;
        branch.left = left;
        branch.right = right;
        branch.weight = nodes_[left].weight + nodes_[right].weight;

        nodes_.push_back(branch);
        const int index = static_cast<int>(nodes_.size()) - 1;
        nodes_[left].parent = index;
        nodes_[right].parent = index;

        if (queue.empty()) {
            root_ = index;
            break;
        }
        insert(index);
    }

    BuildCodeTable();
}

void HuffmanTree::BuildCodeTable() {
    for (std::size_t symbol = 0; symbol < kSymbolCount; ++symbol) {
        std::vector<bool>& code = codes_[symbol];
        code.clear();

        // Codes are discovered walking up from the leaf, so the bits come out
        // reversed and have to be flipped before use.
        for (int node = static_cast<int>(symbol); node != root_;) {
            const int parent = nodes_[node].parent;
            code.push_back(nodes_[parent].left != node);
            node = parent;
        }

        std::reverse(code.begin(), code.end());
    }
}

std::size_t HuffmanTree::Decode(net::BitStream& stream, int bitCount, void* output,
                                std::size_t capacity) const {
    auto* destination = static_cast<std::uint8_t*>(output);
    std::size_t produced = 0;
    int node = root_;

    for (int remaining = bitCount; remaining > 0; --remaining) {
        bool bit = false;
        if (!stream.ReadBit(bit)) {
            break;
        }

        node = bit ? nodes_[node].right : nodes_[node].left;

        if (nodes_[node].IsLeaf()) {
            if (produced < capacity) {
                destination[produced] = nodes_[node].symbol;
            }
            ++produced;
            node = root_;
        }
    }

    return produced;
}

int HuffmanTree::Encode(const void* input, std::size_t size, net::BitStream& stream) const {
    const auto* source = static_cast<const std::uint8_t*>(input);
    int bitsWritten = 0;

    for (std::size_t i = 0; i < size; ++i) {
        for (bool bit : codes_[source[i]]) {
            stream.WriteBit(bit);
            ++bitsWritten;
        }
    }

    return bitsWritten;
}

bool CompressBlock(const void* input, std::size_t size, net::BitStream& out) {
    const auto* bytes = static_cast<const std::uint8_t*>(input);

    HuffmanTree::FrequencyTable frequencies{};
    for (std::size_t i = 0; i < size; ++i) {
        ++frequencies[bytes[i]];
    }

    const HuffmanTree tree(frequencies);

    out.WriteCompressed(static_cast<std::uint32_t>(size));
    for (std::uint32_t frequency : frequencies) {
        out.WriteCompressed(frequency);
    }

    // The encoded run starts on a byte boundary so the length that precedes it
    // can be rewritten in place afterwards.
    out.AlignWriteToByteBoundary();

    const int lengthOffset = out.GetNumberOfBitsUsed();
    out.Write<std::uint32_t>(0);

    const int bitCount = tree.Encode(input, size, out);
    const int endOffset = out.GetNumberOfBitsUsed();

    out.SetWriteOffset(lengthOffset);
    out.Write(static_cast<std::uint32_t>(bitCount));
    out.SetWriteOffset(endOffset);

    return true;
}

bool DecompressBlock(net::BitStream& in, std::vector<std::uint8_t>& out, std::size_t maxSize) {
    out.clear();

    std::uint32_t size = 0;
    if (!in.ReadCompressed(size) || size > maxSize) {
        return false;
    }

    HuffmanTree::FrequencyTable frequencies{};
    for (std::uint32_t& frequency : frequencies) {
        if (!in.ReadCompressed(frequency)) {
            return false;
        }
    }

    in.AlignReadToByteBoundary();

    std::uint32_t bitCount = 0;
    if (!in.Read(bitCount)) {
        return false;
    }

    const HuffmanTree tree(frequencies);

    out.assign(size, 0);
    const std::size_t produced = tree.Decode(in, static_cast<int>(bitCount), out.data(), size);

    // A run that decodes to a different length than the header promised means
    // the block is corrupt, whatever the bits happened to spell.
    if (produced != size) {
        out.clear();
        return false;
    }
    return true;
}

const HuffmanTree& HuffmanTree::Default() {
    static const HuffmanTree tree(kDefaultTextFrequencies);
    return tree;
}

}  // namespace samp::compression
