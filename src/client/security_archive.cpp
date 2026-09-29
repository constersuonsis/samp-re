#include "samp/client/security_archive.h"

#include <cstring>

namespace samp::client {
namespace {

std::uint32_t ReadLittleEndian32(const std::uint8_t *data) {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

void WriteLittleEndian32(std::uint8_t *data, std::uint32_t value) {
  data[0] = static_cast<std::uint8_t>(value);
  data[1] = static_cast<std::uint8_t>(value >> 8);
  data[2] = static_cast<std::uint8_t>(value >> 16);
  data[3] = static_cast<std::uint8_t>(value >> 24);
}

}

SecurityBlockParameters InitializeSecurityBlockParameters(std::uint32_t seed,
                                                          std::uint32_t rounds) {
  const std::uint32_t delta = ((seed ^ 0xC107FFFFu) >> 19) |
                              ((seed ^ 0x0003D3E7u) << 13);
  return {delta * rounds, delta, rounds};
}

void CopySecurityBlock(std::array<std::uint32_t, 4> &destination,
                       const std::uint8_t *source, std::uint8_t xor_value) {
  auto *bytes = reinterpret_cast<std::uint8_t *>(destination.data());
  std::memcpy(bytes, source, sizeof(destination));
  if (xor_value != 0) {
    for (std::size_t index = 0; index < sizeof(destination); ++index) {
      bytes[index] ^= xor_value;
    }
  }
}

bool DecryptSecurityBlocks(std::array<std::uint32_t, 4> &state, std::uint32_t initial_sum,
                           std::uint32_t delta, std::uint32_t rounds, std::uint8_t *data,
                           std::size_t size) {
  if ((!data && size != 0) || (size % 8) != 0) {
    return false;
  }
  for (std::size_t offset = 0; offset < size; offset += 8) {
    std::uint32_t first = ReadLittleEndian32(data + offset);
    std::uint32_t second = ReadLittleEndian32(data + offset + 4);
    const std::uint32_t original_first = first;
    const std::uint32_t original_second = second;
    std::uint32_t sum = initial_sum;
    for (std::uint32_t iteration = 0; iteration < rounds; ++iteration) {
      second -= (sum + state[(sum >> 11) & 3]) ^
                (first + ((first << 4) ^ (first >> 5)));
      sum -= delta;
      first -= (sum + state[sum & 3]) ^ (second + ((second << 4) ^ (second >> 5)));
    }
    WriteLittleEndian32(data + offset, first);
    WriteLittleEndian32(data + offset + 4, second);
    state[0] ^= original_first;
    state[1] ^= original_second;
    state[2] ^= original_first;
    state[3] ^= original_second;
  }
  return true;
}

}
