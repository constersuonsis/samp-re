#include "samp/client/settings.h"
#include "samp/util/ini_store.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>

namespace samp::client {
namespace {

ClientSettings g_settings;
samp::util::IniStore g_config;
samp::util::IniStore g_session_config;

}  // namespace

void InitClientSettings(ClientSettings &settings) {
  settings.table_link = 0;
  settings.active = 0;
  settings.input_limit = 120;
  settings.message_limit = 256;
  settings.directory[0] = 0;
}

bool LoadClientSettings(ClientSettings &settings, const char *directory) {
  InitClientSettings(settings);
  if (!directory) {
    return false;
  }
  size_t length = 0;
  while (directory[length] != 0 && length + 1 < sizeof(settings.directory)) {
    settings.directory[length] = directory[length];
    ++length;
  }
  settings.directory[length] = 0;
  settings.active = 1;
  char config_path[MAX_PATH] = {};
  sprintf_s(config_path, "%s\\sa-mp.cfg", settings.directory);
  samp::util::InitFromFile(g_config, config_path);
  return true;
}

ClientSettings &SharedSettings() {
  return g_settings;
}

int GetSettingInt(const char *name, int fallback) {
  int index = samp::util::FindEntry(g_config, name);
  if (index < 0) {
    return fallback;
  }
  return g_config.entries[index].int_value;
}

const char *GetSettingString(const char *name) {
  return samp::util::GetString(g_config, name);
}

bool LoadSessionSettings(const char *path) {
  return samp::util::InitFromFile(g_session_config, path);
}

int GetSessionSettingInt(const char *name) {
  int index = samp::util::FindEntry(g_session_config, name);
  if (index < 0 || g_session_config.entries[index].type != 1) {
    return 0;
  }
  return g_session_config.entries[index].int_value;
}

bool HasSessionSetting(const char *name) {
  return samp::util::FindEntry(g_session_config, name) >= 0;
}

bool SetSessionSettingInt(const char *name, int value) {
  if (!name || !name[0]) {
    return false;
  }
  samp::util::SetIntValue(g_session_config, name, value);
  return samp::util::SaveToFile(g_session_config);
}

}  // namespace samp::client
