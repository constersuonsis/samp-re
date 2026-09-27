#include "samp/game/camera_presets.h"

#include <cstddef>
#include <cstring>

namespace samp::game {
namespace {

unsigned char g_preset_flags[0x30] = {};
unsigned char g_preset_matrix[0x2760] = {};
unsigned char g_preset_types[0xD0] = {};
unsigned g_preset_scales[0x906] = {};
unsigned g_preset_fov[0xD2] = {};
unsigned g_preset_active[0xD2] = {};

void FillDwords(unsigned *target, size_t count, unsigned value) {
  for (size_t i = 0; i < count; ++i) {
    target[i] = value;
  }
}

}  // namespace

void InitCameraPresets() {  std::memset(g_preset_flags, 0, sizeof(g_preset_flags));
  std::memset(g_preset_matrix, 0, sizeof(g_preset_matrix));
  std::memset(g_preset_types, 4, sizeof(g_preset_types));
  unsigned short marker = 1028;
  std::memcpy(g_preset_types + 207, &marker, sizeof(marker));
  FillDwords(g_preset_scales, 0x906, 1148829696);
  FillDwords(g_preset_fov, 0xD2, 1051372191);
  FillDwords(g_preset_active, 0xD2, 1065353216);
}

unsigned WeaponPresetValue(unsigned id, unsigned slot) {
  size_t index = static_cast<size_t>(id) * 11 + slot;
  if (index >= 0x906) {
    return 1148829696;
  }
  return g_preset_scales[index];
}

}  // namespace samp::game
