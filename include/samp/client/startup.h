#pragma once

#include <windows.h>

namespace samp::client {

struct LaunchParams {
  bool connect_primary = false;
  bool connect_fallback = false;
  char password[257] = {};
  char host[257] = {};
  char port[257] = {};
  char player_name[257] = {};
  char debug_script[260] = {};
};

char ParseCommandLine();
const LaunchParams &CurrentLaunchParams();
HINSTANCE ModuleInstance();

bool HandleAttach(HINSTANCE module);
void HandleDetach();

void SetModuleDirectoryFromModulePath(char *path);

}  // namespace samp::client
