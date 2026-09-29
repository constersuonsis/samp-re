#pragma once

#include <array>
#include <cstddef>

namespace samp::client {

inline constexpr std::size_t kServerModelBlockSize = 212;

void DoSpawn();
void ResetSpawnView(void *status);
void SetSpawnViewBounds();
void SetSpawnViewZoom();
void ApplyServerWeather(int weather);
std::size_t ApplyServerModelSettings(const std::array<char, kServerModelBlockSize> &model_priorities,
                                     std::array<bool, kServerModelBlockSize> &loaded_models);

}  // namespace samp::client
