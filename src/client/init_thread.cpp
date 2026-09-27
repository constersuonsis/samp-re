#include "samp/client/init_thread.h"
#include "samp/game/animation_ids.h"
#include "samp/game/camera_presets.h"
#include "samp/game/hooks.h"
#include "samp/game/patches.h"
#include "samp/game/anticheat.h"
#include "samp/game/version.h"
#include "samp/player/player_colors.h"
#include "samp/scripting/command_manager.h"
#include "samp/util/logger.h"

#include <windows.h>

#include <cstdlib>

namespace samp::client {
namespace {

bool g_controller_present = false;
char *g_runtime_flags = nullptr;

void ResetPlayerColors() {
  samp::player::ResetPlayerColors();
}

void InitCameraPresets() {
  samp::game::InitCameraPresets();
}

void InitPlayerColors() {
  samp::player::InitPlayerColors();
}

int DetectGameVersion() {
  return samp::game::DetectGameVersion();
}

int InitializeClient() {
  g_runtime_flags = static_cast<char *>(std::calloc(1, 0x201));
  samp::util::WriteLogLine("runtime flags allocated");
  ResetPlayerColors();
  samp::util::WriteLogLine("player colors reset");
  InitCameraPresets();
  samp::util::WriteLogLine("camera presets ready");
  InitPlayerColors();
  samp::util::WriteLogLine("player colors loaded");
  int detected = DetectGameVersion();
  samp::util::WriteLogLine(detected ? "game version detected" : "game version unknown");
  if (!detected) {
    ::MessageBoxA(nullptr,
                  "I can't determine your GTA version.\r\nSA-MP only supports GTA:SA v1.0 USA/EU",
                  "Version Error", MB_ICONEXCLAMATION);
    ::ExitProcess(1);
  }
  return detected;
}

volatile LONG *GameStateCell() {
  return reinterpret_cast<volatile LONG *>(0xC8D4C0);
}

void WaitForPlayableState() {
  while (*GameStateCell() != 7) {
    ::Sleep(5);
  }
}

void ArmAnticheat() {
  samp::game::SetAntiCheatFlag();
}

void ApplyAllPatches() {
  samp::game::ApplyAllPatches();
}

void PatchGameHooks() {
  samp::game::PatchAllHooks();
}

void InitMenuResources() {
  samp::scripting::CreateCommandManager();
}

void InitWeaponModels() {
  samp::game::BuildAnimationIdTable();
}

void FinalizeGameInit() {
  ApplyAllPatches();
  samp::util::WriteLogLine("game patches applied");
  PatchGameHooks();
  samp::util::WriteLogLine("game hooks installed");
  InitMenuResources();
  samp::util::WriteLogLine("menu resources ready");
  InitWeaponModels();
  samp::util::WriteLogLine("model table ready");
  *reinterpret_cast<volatile unsigned char *>(0xC8D4C0) = 8;
  *reinterpret_cast<volatile unsigned char *>(0xBA6831) = 1;
  *reinterpret_cast<volatile unsigned char *>(0xBA67A4) = 0;
  *reinterpret_cast<volatile unsigned char *>(0xBA677B) = 0;
}

}  // namespace

bool HasMainController() {
  return g_controller_present;
}

void SetMainControllerPresent(bool present) {
  g_controller_present = present;
}

unsigned RunInitThread() {
  samp::util::WriteLogLine("init thread begin");
  if (HasMainController()) {
    InitializeClient();
    samp::util::WriteLogLine("client initialized");
  }
  WaitForPlayableState();
  samp::util::WriteLogLine("game playable");
  ArmAnticheat();
  FinalizeGameInit();
  samp::util::WriteLogLine("game initialized");
  return 0;
}

}  // namespace samp::client
