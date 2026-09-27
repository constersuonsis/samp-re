#pragma once

namespace samp::game {

void SetWeaponBlock(void *block);
void SaveWeaponModels();
void LoadWeaponPreset(int id);
void RestoreWeaponModels();
void ResetWeaponModels();
unsigned WeaponPresetValue(unsigned id, unsigned slot);

}  // namespace samp::game
