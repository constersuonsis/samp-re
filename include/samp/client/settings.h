#pragma once

namespace samp::client {

struct ClientSettings {
  unsigned table_link = 0;
  unsigned char active = 0;
  unsigned input_limit = 0;
  unsigned message_limit = 0;
  char directory[260] = {};
};

void InitClientSettings(ClientSettings &settings);
bool LoadClientSettings(ClientSettings &settings, const char *directory);
ClientSettings &SharedSettings();
int GetSettingInt(const char *name, int fallback);
const char *GetSettingString(const char *name);
bool LoadSessionSettings(const char *path);
int GetSessionSettingInt(const char *name);
bool HasSessionSetting(const char *name);
bool SetSessionSettingInt(const char *name, int value);

}  // namespace samp::client
