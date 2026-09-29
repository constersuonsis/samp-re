#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace samp::client {

inline constexpr std::size_t kServerModelBlockSize = 212;

struct ServerSpawnInfo {
  std::uint8_t team = 0;
  std::uint32_t model_id = 0;
  std::uint8_t reserved = 0;
  std::array<float, 3> position{};
  float rotation = 0.0f;
  std::array<std::uint32_t, 3> weapons{};
  std::array<std::uint32_t, 3> ammunition{};
};

void DoSpawn();
bool ApplyServerSpawnInfo(const ServerSpawnInfo &spawn_info);
void ResetSpawnView(void *status);
void SetSpawnViewBounds();
void SetSpawnViewZoom();
void ApplyServerWeather(int weather);
std::size_t ApplyServerModelSettings(const std::array<char, kServerModelBlockSize> &model_priorities,
                                     std::array<bool, kServerModelBlockSize> &loaded_models);

}  // namespace samp::client
