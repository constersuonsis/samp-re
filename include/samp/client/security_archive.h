#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::client {

struct SecurityBlockParameters {
  std::uint32_t initial_sum = 0;
  std::uint32_t delta = 0;
  std::uint32_t rounds = 0;
};

SecurityBlockParameters InitializeSecurityBlockParameters(std::uint32_t seed,
                                                          std::uint32_t rounds);
void CopySecurityBlock(std::array<std::uint32_t, 4> &destination, const std::uint8_t *source,
                       std::uint8_t xor_value);
bool DecryptSecurityBlocks(std::array<std::uint32_t, 4> &state, std::uint32_t initial_sum,
                           std::uint32_t delta, std::uint32_t rounds, std::uint8_t *data,
                           std::size_t size);

}
