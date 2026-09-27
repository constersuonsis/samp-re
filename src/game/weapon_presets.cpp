#include "samp/game/weapon_presets.h"
#include "samp/game/camera_presets.h"

#include <cstddef>
#include <cstring>

namespace samp::game {
namespace {

unsigned g_saved_models[11] = {};
void *g_weapon_block = nullptr;

unsigned PresetTableValue(unsigned id, unsigned slot) {
  return WeaponPresetValue(id, slot);
}

}  // namespace

void SetWeaponBlock(void *block) {
  g_weapon_block = block;
}

void SaveWeaponModels() {
  if (!g_weapon_block) {
    return;
  }
  std::memcpy(g_saved_models, static_cast<const unsigned char *>(g_weapon_block) + 276,
              sizeof(g_saved_models));
}

void LoadWeaponPreset(int id) {
  if (!g_weapon_block || id < 0) {
    return;
  }
  unsigned char *block = static_cast<unsigned char *>(g_weapon_block);
  for (unsigned slot = 0; slot < 11; ++slot) {
    unsigned value = PresetTableValue(static_cast<unsigned>(id), slot);
    std::memcpy(block + 276 + slot * 4, &value, sizeof(value));
  }
}

void RestoreWeaponModels() {
  if (!g_weapon_block) {
    return;
  }
  std::memcpy(static_cast<unsigned char *>(g_weapon_block) + 276, g_saved_models,
              sizeof(g_saved_models));
}

void ResetWeaponModels() {
  if (!g_weapon_block) {
    SaveWeaponModels();
    return;
  }
  unsigned char *block = static_cast<unsigned char *>(g_weapon_block);
  for (unsigned slot = 0; slot < 11; ++slot) {
    unsigned value = 1148829696;
    std::memcpy(block + 276 + slot * 4, &value, sizeof(value));
  }
  SaveWeaponModels();
}

}  // namespace samp::game
