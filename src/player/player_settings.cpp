#include "samp/player/player_settings.h"

#include <cstddef>
#include <cstring>

namespace samp::player {
namespace {

struct SettingsRecord {
  unsigned char values[0x134] = {};
};

void *g_game_block = nullptr;
unsigned char g_backup[0x134] = {};
unsigned char g_backup_a = 0;
unsigned char g_backup_b = 0;
SettingsRecord g_records[64] = {};

}  // namespace

void SetGameSettingsBlock(void *block) {
  g_game_block = block;
}

void *GameSettingsBlock() {
  return g_game_block;
}

void *SettingsBackup() {
  return g_backup;
}

void *SettingsByID(int id) {
  if (id < 0 || id >= 64) {
    return nullptr;
  }
  return g_records[id].values;
}

void SavePlayerSettings() {
  if (!g_game_block) {
    return;
  }
  std::memcpy(g_backup, g_game_block, sizeof(g_backup));
  const unsigned char *game = static_cast<const unsigned char *>(g_game_block);
  g_backup_a = game[4];
  g_backup_b = game[8];
}

void RestorePlayerSettings() {
  if (!g_game_block) {
    return;
  }
  std::memcpy(g_game_block, g_backup, sizeof(g_backup));
  unsigned char *game = static_cast<unsigned char *>(g_game_block);
  game[4] = g_backup_a;
  game[8] = g_backup_b;
}

bool LoadPlayerSettingsByID(int id) {
  if (!g_game_block || id < 0 || id >= 64) {
    return false;
  }
  std::memcpy(g_game_block, g_records[id].values, sizeof(g_backup));
  unsigned short flag_a = 0;
  unsigned short flag_b = 0;
  std::memcpy(&flag_a, g_records[id].values + 10, sizeof(flag_a));
  std::memcpy(&flag_b, g_records[id].values + 12, sizeof(flag_b));
  unsigned char *game = static_cast<unsigned char *>(g_game_block);
  game[4] = flag_a != 0;
  game[8] = flag_b != 0;
  return flag_b != 0;
}

void ClearPlayerSettingsByID(int id) {
  if (id < 0 || id >= 64) {
    return;
  }
  std::memset(g_records[id].values, 0, sizeof(g_backup));
}

}  // namespace samp::player
