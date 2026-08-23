#include "samp/compression/huffman_tree.h"
#include "samp/compression/text_frequencies.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

void TestDefaultFrequenciesMatchReference() {
    using samp::compression::kDefaultTextFrequencies;

    long long total = 0;
    for (const int frequency : kDefaultTextFrequencies) {
        total += frequency;
    }
    Check(total == 65380, "the default table checksum matches the reference");
    Check(kDefaultTextFrequencies[32] == 11084, "space is the most common symbol");
    Check(kDefaultTextFrequencies[10] == 722 && kDefaultTextFrequencies[13] == 2,
          "line endings keep their reference weights");
    Check(kDefaultTextFrequencies[37] == 31, "percent keeps its reference weight");
}

samp::compression::HuffmanTree::FrequencyTable FrequenciesOf(const std::string& sample) {
    samp::compression::HuffmanTree::FrequencyTable frequencies{};
    for (unsigned char c : sample) {
        ++frequencies[c];
    }
    return frequencies;
}

void TestRoundTrip() {
    const std::string sample = "the quick brown fox jumps over the lazy dog";
    const samp::compression::HuffmanTree tree(FrequenciesOf(sample));

    samp::net::BitStream stream;
    const int bits = tree.Encode(sample.data(), sample.size(), stream);
    Check(bits > 0, "encoding produces bits");

    char decoded[128] = {};
    const std::size_t produced = tree.Decode(stream, bits, decoded, sizeof(decoded));

    Check(produced == sample.size(), "decoding produces the original length");
    Check(std::string(decoded, produced) == sample, "text round-trips through the tree");
}

void TestCompressionActuallyShrinks() {
    const std::string sample(200, 'a');
    const samp::compression::HuffmanTree tree(FrequenciesOf(sample));

    samp::net::BitStream stream;
    const int bits = tree.Encode(sample.data(), sample.size(), stream);

    Check(bits < static_cast<int>(sample.size()) * 8,
          "a repetitive sample costs less than one byte per symbol");
}

void TestEveryByteStaysEncodable() {

    const samp::compression::HuffmanTree::FrequencyTable empty{};
    const samp::compression::HuffmanTree tree(empty);

    for (int symbol = 0; symbol < 256; ++symbol) {
        if (tree.GetCodeLength(static_cast<std::uint8_t>(symbol)) == 0) {
            Check(false, "every byte value has a code even with no frequency data");
            return;
        }
    }

    const auto singleByte = static_cast<std::uint8_t>('Z');
    Check(tree.GetCodeLength(singleByte) == 8, "a flat table yields fixed-width codes");
}

void TestDecodeReportsLengthBeyondCapacity() {
    const std::string sample = "aaaaaaaaaabbbbbbbbbb";
    const samp::compression::HuffmanTree tree(FrequenciesOf(sample));

    samp::net::BitStream stream;
    const int bits = tree.Encode(sample.data(), sample.size(), stream);

    char small[4] = {};
    const std::size_t produced = tree.Decode(stream, bits, small, sizeof(small));

    Check(produced == sample.size(), "decoding reports the full length");
    Check(std::memcmp(small, sample.data(), sizeof(small)) == 0, "the buffer holds the prefix");
    Check(stream.GetNumberOfUnreadBits() == 0, "the whole encoded run is consumed");
}

void TestFrequenciesShapeTheTree() {

    std::string skewed(100, 'x');
    skewed += "qz";

    const samp::compression::HuffmanTree tree(FrequenciesOf(skewed));
    Check(tree.GetCodeLength('x') < tree.GetCodeLength('q'),
          "a frequent symbol gets a shorter code than a rare one");
}

void TestBlockRoundTrip() {
    const std::string sample =
        "Blocks carry their own frequency table, so the decoder needs nothing "
        "agreed beforehand to rebuild the tree.";

    samp::net::BitStream stream;
    Check(samp::compression::CompressBlock(sample.data(), sample.size(), stream),
          "a block compresses");

    std::vector<std::uint8_t> decoded;
    Check(samp::compression::DecompressBlock(stream, decoded), "a block decompresses");
    Check(std::string(decoded.begin(), decoded.end()) == sample, "the block round-trips");
    Check(stream.GetNumberOfUnreadBits() == 0, "the block is consumed exactly");
}

void TestEmptyBlockRoundTrip() {
    samp::net::BitStream stream;
    Check(samp::compression::CompressBlock(nullptr, 0, stream), "an empty block compresses");

    std::vector<std::uint8_t> decoded{1, 2, 3};
    Check(samp::compression::DecompressBlock(stream, decoded), "an empty block decompresses");
    Check(decoded.empty(), "an empty block decodes to nothing");
}

void TestBlockCarriesItsOwnTable() {

    const std::string first(300, 'a');
    std::string second;
    for (int i = 0; i < 300; ++i) {
        second += static_cast<char>('a' + (i % 26));
    }

    samp::net::BitStream stream;
    Check(samp::compression::CompressBlock(first.data(), first.size(), stream), "first packs");
    Check(samp::compression::CompressBlock(second.data(), second.size(), stream), "second packs");

    std::vector<std::uint8_t> decodedFirst;
    std::vector<std::uint8_t> decodedSecond;
    Check(samp::compression::DecompressBlock(stream, decodedFirst), "first unpacks");
    Check(samp::compression::DecompressBlock(stream, decodedSecond), "second unpacks");

    Check(std::string(decodedFirst.begin(), decodedFirst.end()) == first,
          "the repetitive block round-trips");
    Check(std::string(decodedSecond.begin(), decodedSecond.end()) == second,
          "the varied block round-trips under its own table");
}

void TestOversizedClaimIsRefused() {

    samp::net::BitStream stream;
    stream.WriteCompressed<std::uint32_t>(500);

    std::vector<std::uint8_t> decoded;
    Check(!samp::compression::DecompressBlock(stream, decoded, 100),
          "a block claiming more than the ceiling is refused before allocating");
    Check(decoded.empty(), "a refused block leaves nothing behind");
}

void TestCorruptBlockIsRefused() {
    const std::string sample = "the quick brown fox";

    samp::net::BitStream stream;
    Check(samp::compression::CompressBlock(sample.data(), sample.size(), stream), "block packs");

    stream.SetWriteOffset(stream.GetNumberOfBitsUsed() - 8);

    std::vector<std::uint8_t> decoded;
    Check(!samp::compression::DecompressBlock(stream, decoded),
          "a run that decodes short of its declared size is refused");
}

}

int main() {
    TestBlockRoundTrip();
    TestEmptyBlockRoundTrip();
    TestBlockCarriesItsOwnTable();
    TestOversizedClaimIsRefused();
    TestCorruptBlockIsRefused();
    TestRoundTrip();
    TestCompressionActuallyShrinks();
    TestEveryByteStaysEncodable();
    TestDecodeReportsLengthBeyondCapacity();
    TestFrequenciesShapeTheTree();
    TestDefaultFrequenciesMatchReference();

    if (g_failures == 0) {
        std::printf("All Huffman tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
