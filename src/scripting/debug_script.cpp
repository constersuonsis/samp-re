#include "samp/scripting/debug_script.h"

#include <cstdio>
#include <cstring>

namespace samp::scripting {
namespace {

void *g_debug_pool = nullptr;

void SetDebugTimeOfDay() {
}

void ParseScriptLine(const char *line) {
  (void)line;
}

void SpawnDebugObjects() {
}

void AnnounceFormat(const char *format, const char *arg) {
  (void)format;
  (void)arg;
}

}  // namespace

bool LoadDebugScript(const char *path) {
  SetDebugTimeOfDay();
  AnnounceFormat("DEBUGSCRIPT: Loading %s", path);
  g_debug_pool = ::operator new(0x1F44);
  std::memset(g_debug_pool, 0, 0x1F44);
  std::FILE *file = nullptr;
  if (fopen_s(&file, path, "r") != 0 || !file) {
    AnnounceFormat("DEBUGSCRIPT: I can't open %s", path);
    return false;
  }
  char line[256] = {};
  while (std::fgets(line, sizeof(line), file)) {
    ParseScriptLine(line);
  }
  std::fclose(file);
  SpawnDebugObjects();
  return true;
}

}  // namespace samp::scripting
