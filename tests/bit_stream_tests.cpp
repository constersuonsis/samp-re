#include "samp/network/bit_stream.h"

#include <cstdio>
#include <cstdint>
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

void TestBitRoundTrip() {
    samp::net::BitStream stream;
    const std::vector<bool> pattern = {true, false, true, true, false, false, false, true, true};

    for (bool bit : pattern) {
        stream.WriteBit(bit);
    }
    Check(stream.GetNumberOfBitsUsed() == static_cast<int>(pattern.size()), "bit count after writes");

    for (std::size_t i = 0; i < pattern.size(); ++i) {
        bool value = false;
        Check(stream.ReadBit(value), "bit read succeeds");
        Check(value == pattern[i], "bit value round-trips");
    }

    bool overflow = false;
    Check(!stream.ReadBit(overflow), "reading past the end fails");
}

void TestTypedRoundTrip() {
    samp::net::BitStream stream;
    stream.Write<std::uint8_t>(0xAB);
    stream.Write<std::uint16_t>(0x1234);
    stream.Write<std::uint32_t>(0xDEADBEEF);
    stream.Write<float>(3.5f);

    std::uint8_t byteValue = 0;
    std::uint16_t shortValue = 0;
    std::uint32_t intValue = 0;
    float floatValue = 0.0f;

    Check(stream.Read(byteValue) && byteValue == 0xAB, "uint8 round-trips");
    Check(stream.Read(shortValue) && shortValue == 0x1234, "uint16 round-trips");
    Check(stream.Read(intValue) && intValue == 0xDEADBEEF, "uint32 round-trips");
    Check(stream.Read(floatValue) && floatValue == 3.5f, "float round-trips");
}

void TestUnalignedWrites() {
    // A single bit in front of a byte payload forces the shifting path.
    samp::net::BitStream stream;
    stream.WriteBit(true);
    stream.Write<std::uint32_t>(0x11223344);
    stream.WriteBit(false);
    stream.Write<std::uint16_t>(0xF00D);

    bool first = false;
    bool second = true;
    std::uint32_t intValue = 0;
    std::uint16_t shortValue = 0;

    Check(stream.ReadBit(first) && first, "leading bit round-trips");
    Check(stream.Read(intValue) && intValue == 0x11223344, "unaligned uint32 round-trips");
    Check(stream.ReadBit(second) && !second, "middle bit round-trips");
    Check(stream.Read(shortValue) && shortValue == 0xF00D, "unaligned uint16 round-trips");
}

void TestCompressed() {
    samp::net::BitStream stream;
    stream.WriteCompressed<std::uint32_t>(3);
    stream.WriteCompressed<std::uint32_t>(0xDEADBEEF);
    stream.WriteCompressed<std::int32_t>(-2);

    std::uint32_t small = 0;
    std::uint32_t large = 0;
    std::int32_t negative = 0;

    Check(stream.ReadCompressed(small) && small == 3, "small compressed value round-trips");
    Check(stream.ReadCompressed(large) && large == 0xDEADBEEF, "large compressed value round-trips");
    Check(stream.ReadCompressed(negative) && negative == -2, "negative compressed value round-trips");

    // A value that fits in a nibble must cost far less than the full 32 bits.
    samp::net::BitStream compact;
    compact.WriteCompressed<std::uint32_t>(5);
    Check(compact.GetNumberOfBitsUsed() < 32, "compression actually shrinks small values");
}

void TestUnalignedBytes() {
    // WriteBytes packs straight against the preceding bit; ReadBytes must
    // unpack it the same way instead of skipping to the next byte.
    const char payload[] = "sync";
    samp::net::BitStream stream;
    stream.WriteBit(true);
    stream.WriteBytes(payload, sizeof(payload));

    bool flag = false;
    char readBack[sizeof(payload)] = {};
    Check(stream.ReadBit(flag) && flag, "flag before unaligned payload");
    Check(stream.ReadBytes(readBack, sizeof(readBack)), "unaligned payload reads back");
    Check(std::string(readBack) == "sync", "unaligned payload round-trips");
    Check(stream.GetNumberOfBitsUsed() == 1 + 8 * static_cast<int>(sizeof(payload)),
          "unaligned payload adds no padding");
}

void TestAlignedBytes() {
    const char payload[] = "sync";
    samp::net::BitStream stream;
    stream.WriteBit(true);
    stream.WriteAlignedBytes(payload, sizeof(payload));

    bool flag = false;
    char readBack[sizeof(payload)] = {};
    Check(stream.ReadBit(flag) && flag, "flag before aligned payload");
    Check(stream.ReadAlignedBytes(readBack, sizeof(readBack)), "aligned payload reads back");
    Check(std::string(readBack) == "sync", "aligned payload round-trips");
    Check(stream.GetNumberOfBitsUsed() == 8 + 8 * static_cast<int>(sizeof(payload)),
          "aligned payload is padded to a byte boundary");
}

void TestHeapGrowth() {
    // Push well past the internal buffer so the stream has to move to the heap.
    samp::net::BitStream stream;
    const int count = 4096;
    for (int i = 0; i < count; ++i) {
        stream.Write<std::uint32_t>(static_cast<std::uint32_t>(i));
    }

    bool allMatched = true;
    for (int i = 0; i < count; ++i) {
        std::uint32_t value = 0;
        if (!stream.Read(value) || value != static_cast<std::uint32_t>(i)) {
            allMatched = false;
            break;
        }
    }
    Check(allMatched, "values survive reallocation onto the heap");
}

void TestBorrowedBuffer() {
    const std::uint8_t raw[] = {0x80, 0x00, 0x00, 0x00};
    samp::net::BitStream stream(raw, sizeof(raw), false);

    bool bit = false;
    Check(stream.ReadBit(bit) && bit, "first bit of a borrowed buffer");
    Check(stream.GetData() == raw, "borrowed buffer is not copied");
}

}  // namespace

int main() {
    TestBitRoundTrip();
    TestTypedRoundTrip();
    TestUnalignedWrites();
    TestCompressed();
    TestUnalignedBytes();
    TestAlignedBytes();
    TestHeapGrowth();
    TestBorrowedBuffer();

    if (g_failures == 0) {
        std::printf("All BitStream tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
