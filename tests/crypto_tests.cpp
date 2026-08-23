#include "samp/crypto/packet_checksum.h"
#include "samp/crypto/packet_codec.h"
#include "samp/crypto/xtea.h"

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

const samp::crypto::XteaCipher::Key kKey = {0x11, 0x22, 0x33, 0x44};
constexpr std::uint32_t kDelta = 0x9E3779B9;

void TestBlockRoundTrip() {
    const samp::crypto::XteaCipher cipher(kKey, kDelta);

    std::uint32_t left = 0x01234567;
    std::uint32_t right = 0x89ABCDEF;
    const std::uint32_t originalLeft = left;
    const std::uint32_t originalRight = right;

    cipher.EncryptBlock(left, right);
    Check(left != originalLeft || right != originalRight, "encryption changes the block");

    cipher.DecryptBlock(left, right);
    Check(left == originalLeft && right == originalRight, "a block survives the round trip");
}

void TestBufferRoundTrip() {
    const samp::crypto::XteaCipher cipher(kKey, kDelta);

    std::vector<std::uint8_t> payload(64);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i * 7);
    }
    const std::vector<std::uint8_t> original = payload;

    Check(cipher.Encrypt(payload.data(), payload.size()), "a whole-block buffer encrypts");
    Check(payload != original, "the buffer actually changed");

    Check(cipher.Decrypt(payload.data(), payload.size()), "a whole-block buffer decrypts");
    Check(payload == original, "the buffer survives the round trip");
}

void TestPartialBlockIsRefused() {
    const samp::crypto::XteaCipher cipher(kKey, kDelta);

    std::vector<std::uint8_t> payload(12, 0xAB);
    const std::vector<std::uint8_t> original = payload;

    Check(!cipher.Encrypt(payload.data(), payload.size()), "a partial block is refused");
    Check(payload == original, "a refused buffer is left untouched");
}

void TestDifferentKeysDiverge() {
    const samp::crypto::XteaCipher first(kKey, kDelta);
    const samp::crypto::XteaCipher second({0x11, 0x22, 0x33, 0x45}, kDelta);

    std::uint64_t a = 0x0011223344556677ULL;
    std::uint64_t b = a;

    Check(first.Encrypt(&a, sizeof(a)) && second.Encrypt(&b, sizeof(b)), "both ciphers run");
    Check(a != b, "a one-byte key difference changes the output");
}

void TestChecksumIsOrderSensitive() {
    samp::crypto::PacketChecksum forwards;
    forwards.Update("abcd", 4);

    samp::crypto::PacketChecksum backwards;
    backwards.Update("dcba", 4);

    Check(forwards.Value() != backwards.Value(), "the checksum depends on byte order");
}

void TestChecksumIsRepeatable() {
    samp::crypto::PacketChecksum first;
    first.Update("packet body", 11);

    samp::crypto::PacketChecksum chunked;
    chunked.Update("packet", 6);
    chunked.Update(" body", 5);

    Check(first.Value() == chunked.Value(), "chunking the input does not change the result");

    samp::crypto::PacketChecksum reset;
    reset.Update("noise", 5);
    reset.Reset();
    reset.Update("packet body", 11);
    Check(reset.Value() == first.Value(), "resetting restores the starting state");
}

void TestChecksumDetectsSingleBitFlip() {
    std::string body = "the quick brown fox";

    samp::crypto::PacketChecksum original;
    original.Update(body.data(), body.size());

    body[5] = static_cast<char>(body[5] ^ 0x01);

    samp::crypto::PacketChecksum tampered;
    tampered.Update(body.data(), body.size());

    Check(original.Value() != tampered.Value(), "a flipped bit changes the checksum");
}

samp::crypto::PacketCodec MakeCodec() {

    return samp::crypto::PacketCodec(samp::crypto::XteaCipher(kKey, kDelta),
                                     [] { return static_cast<std::uint8_t>(0x5A); });
}

void TestPacketRoundTrip() {
    const samp::crypto::PacketCodec codec = MakeCodec();
    const std::string body = "the payload";

    std::vector<std::uint8_t> packet;
    Check(codec.Encode(body.data(), body.size(), packet), "a packet encodes");
    Check(packet.size() % samp::crypto::XteaCipher::kBlockSize == 0,
          "an encoded packet is a whole number of blocks");
    Check(packet.size() == samp::crypto::PacketCodec::EncodedSize(body.size()),
          "the predicted size matches the real one");

    std::vector<std::uint8_t> decoded;
    Check(codec.Decode(packet.data(), packet.size(), decoded), "the packet decodes");
    Check(std::string(decoded.begin(), decoded.end()) == body, "the payload round-trips");
}

void TestPaddingCoversEveryLength() {
    const samp::crypto::PacketCodec codec = MakeCodec();

    for (std::size_t size = 0; size <= 32; ++size) {
        const std::vector<std::uint8_t> body(size, 0x42);

        std::vector<std::uint8_t> packet;
        if (!codec.Encode(body.data(), body.size(), packet)) {
            Check(false, "every payload length encodes");
            return;
        }

        std::vector<std::uint8_t> decoded;
        if (!codec.Decode(packet.data(), packet.size(), decoded) || decoded != body) {
            Check(false, "every payload length round-trips regardless of padding");
            return;
        }
    }
}

void TestTamperedPacketIsRejected() {
    const samp::crypto::PacketCodec codec = MakeCodec();
    const std::string body = "sensitive";

    std::vector<std::uint8_t> packet;
    Check(codec.Encode(body.data(), body.size(), packet), "a packet encodes");

    packet[packet.size() / 2] ^= 0x01;

    std::vector<std::uint8_t> decoded;
    Check(!codec.Decode(packet.data(), packet.size(), decoded),
          "a tampered packet fails its checksum");
}

void TestMisalignedPacketIsRejected() {
    const samp::crypto::PacketCodec codec = MakeCodec();

    const std::vector<std::uint8_t> truncated(12, 0);
    std::vector<std::uint8_t> decoded;
    Check(!codec.Decode(truncated.data(), truncated.size(), decoded),
          "a packet that is not a whole number of blocks is rejected");
}

void TestPaddingIsHiddenInTheByte() {

    const samp::crypto::PacketCodec codec = MakeCodec();
    const std::vector<std::uint8_t> body(3, 0x11);

    std::vector<std::uint8_t> packet;
    Check(codec.Encode(body.data(), body.size(), packet), "a packet encodes");

    std::vector<std::uint8_t> decoded;
    Check(codec.Decode(packet.data(), packet.size(), decoded), "the packet decodes");
    Check(decoded == body, "filler in the high nibble does not disturb the payload");
}

}

int main() {
    TestPacketRoundTrip();
    TestPaddingCoversEveryLength();
    TestTamperedPacketIsRejected();
    TestMisalignedPacketIsRejected();
    TestPaddingIsHiddenInTheByte();
    TestBlockRoundTrip();
    TestBufferRoundTrip();
    TestPartialBlockIsRefused();
    TestDifferentKeysDiverge();
    TestChecksumIsOrderSensitive();
    TestChecksumIsRepeatable();
    TestChecksumDetectsSingleBitFlip();

    if (g_failures == 0) {
        std::printf("All crypto tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
