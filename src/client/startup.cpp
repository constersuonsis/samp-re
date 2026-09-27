#include "samp/client/startup.h"
#include "samp/client/controller.h"
#include "samp/client/init_thread.h"
#include "samp/client/settings.h"
#include "samp/game/detour.h"
#include "samp/ui/crash_dialog.h"
#include "samp/util/logger.h"

#include <windows.h>
#include <process.h>

namespace samp::client {
namespace {

HINSTANCE g_module = nullptr;
LaunchParams g_params;
char g_module_directory[MAX_PATH] = {};

LONG WINAPI TopLevelFilter(EXCEPTION_POINTERS *info) {
  samp::util::WriteLogException(info->ExceptionRecord->ExceptionCode,
                                static_cast<unsigned>(reinterpret_cast<uintptr_t>(
                                    info->ExceptionRecord->ExceptionAddress)));
  samp::ui::SaveCrashContext(info->ContextRecord);
  samp::ui::ShowCrashDialog(g_module);
  return EXCEPTION_EXECUTE_HANDLER;
}

bool LoadSettings(const char *directory) {
  return LoadClientSettings(SharedSettings(), directory);
}

bool HookCursor() {
  return samp::game::HookShowCursor();
}

bool AllocateController() {
  static ClientController controller;
  InitClientController(controller);
  SetMainController(&controller);
  SetMainControllerPresent(true);
  return HasMainController();
}

unsigned __stdcall InitThreadMain(void *) {
  return RunInitThread();
}

}  // namespace

void CopyToken(const char *&cursor, char *target, size_t capacity) {
  while (*cursor == ' ') {
    ++cursor;
  }
  if (!*cursor) {
    target[0] = 0;
    return;
  }
  size_t length = 0;
  while (cursor[length] && cursor[length] != ' ' && cursor[length] != '-' &&
         cursor[length] != '/' && length + 1 < capacity) {
    target[length] = cursor[length];
    ++length;
  }
  target[length] = 0;
}

void CopyQuoted(const char *&cursor, char *target, size_t capacity) {
  while (*cursor && *cursor != '"') {
    ++cursor;
  }
  if (*cursor) {
    ++cursor;
  }
  if (!*cursor) {
    target[0] = 0;
    return;
  }
  size_t length = 0;
  while (cursor[length] && cursor[length] != '"' && length + 1 < capacity) {
    target[length] = cursor[length];
    ++length;
  }
  target[length] = 0;
}

char ParseCommandLine() {
  const char *line = ::GetCommandLineA();
  g_params = LaunchParams{};
  if (!line) {
    return 0;
  }
  const char *cursor = line;
  while (*cursor) {
    if (*cursor != '-' && *cursor != '/') {
      ++cursor;
      continue;
    }
    char key = cursor[1];
    ++cursor;
    switch (key) {
      case 'c':
        g_params.connect_fallback = true;
        g_params.connect_primary = false;
        break;
      case 'd':
        g_params.connect_primary = true;
        g_params.connect_fallback = false;
        break;
      case 'z':
        ++cursor;
        CopyToken(cursor, g_params.password, sizeof(g_params.password));
        break;
      case 'h':
        ++cursor;
        CopyToken(cursor, g_params.host, sizeof(g_params.host));
        break;
      case 'p':
        ++cursor;
        CopyToken(cursor, g_params.port, sizeof(g_params.port));
        break;
      case 'n':
        ++cursor;
        CopyToken(cursor, g_params.player_name, sizeof(g_params.player_name));
        break;
      case 'l':
        ++cursor;
        CopyQuoted(cursor, g_params.debug_script, sizeof(g_params.debug_script));
        break;
      default:
        break;
    }
    if (*cursor) {
      ++cursor;
    }
  }
  return line[0];
}

const LaunchParams &CurrentLaunchParams() {
  return g_params;
}

HINSTANCE ModuleInstance() {
  return g_module;
}

void SetModuleDirectoryFromModulePath(char *path) {
  if (!path) {
    return;
  }
  size_t length = 0;
  while (path[length] != 0) {
    ++length;
  }
  while (length > 0 && path[length - 1] != '\\') {
    --length;
  }
  if (length >= sizeof(g_module_directory)) {
    length = sizeof(g_module_directory) - 1;
  }
  for (size_t i = 0; i < length; ++i) {
    g_module_directory[i] = path[i];
  }
  g_module_directory[length] = 0;
}

bool HandleAttach(HINSTANCE module) {
  g_module = module;
  ParseCommandLine();
  if (!g_params.connect_primary && !g_params.connect_fallback) {
    return true;
  }
  ::SetUnhandledExceptionFilter(TopLevelFilter);
  char path[MAX_PATH] = {};
  ::GetModuleFileNameA(module, path, MAX_PATH);
  SetModuleDirectoryFromModulePath(path);
  samp::util::InitLogger(g_module_directory);
  samp::util::WriteLogLine("attach begin");
  samp::util::WriteLogLine("launcher params present");
  samp::util::WriteLogValue("host", g_params.host);
  samp::util::WriteLogValue("port", g_params.port);
  samp::util::WriteLogValue("name", g_params.player_name);
  samp::util::WriteLogNumber("primary", g_params.connect_primary ? 1 : 0);
  if (!LoadSettings(g_module_directory)) {
    samp::util::WriteLogLine("settings failed");
    return false;
  }
  samp::util::WriteLogLine("settings ok");
  ::AddFontResourceA("gtaweap3.ttf");
  ::AddFontResourceA("sampaux3.ttf");
  samp::util::WriteLogLine("fonts added");
  if (!HookCursor()) {
    samp::util::WriteLogLine("cursor hook failed");
  } else {
    samp::util::WriteLogLine("cursor ok");
  }
  if (!AllocateController()) {
    samp::util::WriteLogLine("controller failed");
    return false;
  }
  samp::util::WriteLogLine("controller ok");
  uintptr_t thread = ::_beginthreadex(nullptr, 0, InitThreadMain, nullptr, 0, nullptr);
  if (!thread) {
    samp::util::WriteLogLine("thread failed");
    return false;
  }
  ::CloseHandle(reinterpret_cast<HANDLE>(thread));
  samp::util::WriteLogLine("attach done");
  return true;
}

void HandleDetach() {
  samp::util::WriteLogLine("detach");
}

}  // namespace samp::client
