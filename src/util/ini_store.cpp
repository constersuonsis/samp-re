#include "samp/util/ini_store.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace samp::util {
namespace {

bool IsBlank(char c) {
  return c == ' ' || c == '\t';
}

bool IsKeyEnd(char c) {
  return c == 0 || c == ' ' || c == '=' || c == '\n' || c == '\t' || c == ';';
}

void ToLower(char *text) {
  for (size_t i = 0; text[i] != 0; ++i) {
    text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
  }
}

int DetectType(const char *value) {
  if (!value || !std::strlen(value)) {
    return 0;
  }
  size_t length = std::strlen(value);
  if (value[0] == '"' && value[length - 1] == '"') {
    return 2;
  }
  return std::strchr(value, '.') != nullptr ? 3 : 1;
}

}  // namespace

void ClearStore(IniStore &store) {
  for (int i = 0; i < 512; ++i) {
    store.entries[i] = IniEntry{};
  }
  store.count = 0;
  store.path[0] = 0;
}

int FindEntry(IniStore &store, const char *name) {
  char key[60] = {};
  size_t length = 0;
  while (name[length] != 0 && length + 1 < sizeof(key)) {
    key[length] = name[length];
    ++length;
  }
  key[length] = 0;
  ToLower(key);
  for (int i = 0; i < store.count; ++i) {
    if (store.entries[i].used && std::strcmp(key, store.entries[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

int AddEntry(IniStore &store, const char *name) {
  int index = FindEntry(store, name);
  if (index >= 0) {
    return index;
  }
  if (store.count >= 512) {
    return -1;
  }
  size_t length = 0;
  while (name[length] != 0 && length + 1 < sizeof(store.entries[0].name)) {
    store.entries[store.count].name[length] = name[length];
    ++length;
  }
  store.entries[store.count].name[length] = 0;
  ToLower(store.entries[store.count].name);
  store.entries[store.count].used = true;
  return store.count++;
}

void SetIntValue(IniStore &store, const char *name, int value) {
  int index = AddEntry(store, name);
  if (index < 0) {
    return;
  }
  store.entries[index].int_value = value;
  store.entries[index].type = 1;
}

void SetStringValue(IniStore &store, const char *name, const char *value) {
  int index = AddEntry(store, name);
  if (index < 0) {
    return;
  }
  size_t length = 0;
  while (value[length] != 0 && length + 1 < sizeof(store.entries[0].string_value)) {
    store.entries[index].string_value[length] = value[length];
    ++length;
  }
  store.entries[index].string_value[length] = 0;
  store.entries[index].type = 2;
}

void SetFloatValue(IniStore &store, const char *name, float value) {
  int index = AddEntry(store, name);
  if (index < 0) {
    return;
  }
  store.entries[index].float_value = value;
  store.entries[index].type = 3;
}

void SetAutoDetect(IniStore &store, const char *name, const char *value) {
  int kind = DetectType(value);
  if (kind == 1) {
    SetIntValue(store, name, std::atoi(value));
  } else if (kind == 2) {
    size_t length = std::strlen(value);
    char unquoted[256] = {};
    for (size_t i = 1; i + 1 < length && i < sizeof(unquoted); ++i) {
      unquoted[i - 1] = value[i];
    }
    SetStringValue(store, name, unquoted);
  } else if (kind == 3) {
    SetFloatValue(store, name, static_cast<float>(std::atof(value)));
  }
}

bool LoadFromFile(IniStore &store) {
  std::FILE *file = nullptr;
  if (fopen_s(&file, store.path, "r") != 0 || !file) {
    return false;
  }
  char line[256] = {};
  while (std::fgets(line, sizeof(line), file)) {
    const char *cursor = line;
    while (IsBlank(*cursor)) {
      ++cursor;
    }
    if (*cursor == 0 || *cursor == ';' || *cursor == '\n' || *cursor == '[') {
      continue;
    }
    char key[60] = {};
    int key_length = 0;
    while (!IsKeyEnd(*cursor) && key_length + 1 < static_cast<int>(sizeof(key))) {
      key[key_length++] = static_cast<char>(std::toupper(static_cast<unsigned char>(*cursor)));
      ++cursor;
    }
    key[key_length] = 0;
    if (!key_length) {
      continue;
    }
    while (IsBlank(*cursor)) {
      ++cursor;
    }
    if (*cursor != '=') {
      continue;
    }
    do {
      ++cursor;
    } while (*cursor == ' ' || *cursor == '\t');
    if (!*cursor) {
      continue;
    }
    char value[256] = {};
    int value_length = 0;
    while (*cursor && *cursor != '\n' && value_length + 1 < static_cast<int>(sizeof(value))) {
      value[value_length++] = *cursor++;
    }
    value[value_length] = 0;
    while (value_length > 0 &&
           (value[value_length - 1] == ' ' || value[value_length - 1] == '\t' ||
            value[value_length - 1] == '\r')) {
      value[--value_length] = 0;
    }
    if (value_length) {
      SetAutoDetect(store, key, value);
    }
  }
  std::fclose(file);
  return true;
}

bool InitFromFile(IniStore &store, const char *path) {
  ClearStore(store);
  if (!path || !path[0]) {
    return true;
  }
  size_t length = 0;
  while (path[length] != 0 && length + 1 < sizeof(store.path)) {
    store.path[length] = path[length];
    ++length;
  }
  store.path[length] = 0;
  return LoadFromFile(store);
}

bool SaveToFile(const IniStore &store) {
  if (!store.path[0]) {
    return false;
  }
  std::FILE *file = nullptr;
  if (fopen_s(&file, store.path, "w") != 0 || !file) {
    return false;
  }
  for (int index = 0; index < store.count; ++index) {
    const IniEntry &entry = store.entries[index];
    if (!entry.used) {
      continue;
    }
    if (entry.type == 1) {
      std::fprintf(file, "%s=%d\n", entry.name, entry.int_value);
    } else if (entry.type == 2) {
      std::fprintf(file, "%s=\"%s\"\n", entry.name, entry.string_value);
    } else if (entry.type == 3) {
      std::fprintf(file, "%s=%f\n", entry.name, entry.float_value);
    }
  }
  return std::fclose(file) == 0;
}

const char *GetString(IniStore &store, const char *name) {
  int index = FindEntry(store, name);
  if (index < 0) {
    return "";
  }
  return store.entries[index].string_value;
}

}  // namespace samp::util
