#pragma once

namespace samp::player {

void SetGameSettingsBlock(void *block);
void *GameSettingsBlock();
void *SettingsBackup();
void *SettingsByID(int id);
void SavePlayerSettings();
void RestorePlayerSettings();
bool LoadPlayerSettingsByID(int id);
void ClearPlayerSettingsByID(int id);

}  // namespace samp::player
