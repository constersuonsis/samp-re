#pragma once

namespace samp::util {

struct IniEntry {
  char name[60] = {};
  bool used = false;
  int int_value = 0;
  char string_value[256] = {};
  float float_value = 0.0f;
  int type = 0;
};

struct IniStore {
  IniEntry entries[512] = {};
  int count = 0;
  char path[260] = {};
};

void ClearStore(IniStore &store);
int FindEntry(IniStore &store, const char *name);
int AddEntry(IniStore &store, const char *name);
void SetIntValue(IniStore &store, const char *name, int value);
void SetStringValue(IniStore &store, const char *name, const char *value);
void SetFloatValue(IniStore &store, const char *name, float value);
void SetAutoDetect(IniStore &store, const char *name, const char *value);
bool LoadFromFile(IniStore &store);
bool InitFromFile(IniStore &store, const char *path);
bool SaveToFile(const IniStore &store);
const char *GetString(IniStore &store, const char *name);

}  // namespace samp::util
