#include "samp/util/filesystem.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace samp::util {

bool FileExists(const char *path) {
  struct _stat info = {};
  return ::_stat(path, &info) == 0;
}

bool EnsureSampDirectories(char *buffer, size_t capacity) {
  if (!buffer || !capacity) {
    return false;
  }
  std::memset(buffer, 0, capacity);
  const char *user_files = *reinterpret_cast<const char *const *>(0xC92368);
  if (!user_files || !user_files[0]) {
    ::GetCurrentDirectoryA(static_cast<DWORD>(capacity), buffer);
    return true;
  }
  size_t length = 0;
  while (user_files[length] != 0 && length + 6 < capacity) {
    buffer[length] = user_files[length];
    ++length;
  }
  const char *suffix = "\\SAMP";
  size_t i = 0;
  while (suffix[i] != 0 && length + 1 < capacity) {
    buffer[length++] = suffix[i++];
  }
  buffer[length] = 0;
  if (!FileExists(buffer)) {
    ::CreateDirectoryA(buffer, nullptr);
  }
  char screens[MAX_PATH] = {};
  sprintf_s(screens, "%s\\screens", buffer);
  if (!FileExists(screens)) {
    ::CreateDirectoryA(screens, nullptr);
  }
  char cache[MAX_PATH] = {};
  sprintf_s(cache, "%s\\cache", buffer);
  HKEY key = nullptr;
  if (::RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\SAMP", 0, KEY_READ, &key) ==
      ERROR_SUCCESS) {
    char configured[MAX_PATH] = {};
    DWORD size = sizeof(configured);
    DWORD type = 0;
    if (::RegQueryValueExA(key, "model_cache", nullptr, &type,
                           reinterpret_cast<BYTE *>(configured), &size) == ERROR_SUCCESS &&
        configured[0]) {
      sprintf_s(cache, "%s", configured);
    }
    ::RegCloseKey(key);
  }
  if (!FileExists(cache)) {
    ::CreateDirectoryA(cache, nullptr);
  }
  char local[MAX_PATH] = {};
  sprintf_s(local, "%s\\local", cache);
  if (!FileExists(local)) {
    ::CreateDirectoryA(local, nullptr);
  }
  return true;
}

}  // namespace samp::util
