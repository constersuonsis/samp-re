#include "samp/client/session.h"
#include "samp/audio/audio.h"
#include "samp/client/controller.h"
#include "samp/client/network_client.h"
#include "samp/client/startup.h"
#include "samp/client/settings.h"
#include "samp/game/boot.h"
#include "samp/game/anticheat.h"
#include "samp/game/frame.h"
#include "samp/game/memory.h"
#include "samp/scripting/debug_script.h"
#include "samp/ui/chat.h"
#include "samp/ui/dialog.h"
#include "samp/ui/font.h"
#include "samp/ui/game_window.h"
#include "samp/ui/render_device.h"
#include "samp/ui/renderer.h"
#include "samp/ui/scoreboard.h"
#include "samp/ui/timer.h"
#include "samp/ui/display.h"
#include "samp/ui/world_ui.h"
#include "samp/util/filesystem.h"
#include "samp/util/logger.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace samp::client {
namespace {

bool g_session_ready = false;
void *g_game_device = nullptr;
void *g_font = nullptr;
void *g_chat = nullptr;
void *g_second_chat = nullptr;
unsigned char *g_text_buffer_a = nullptr;
unsigned char *g_text_buffer_b = nullptr;
int g_frame_limiter = 48;
int g_sprite_arg = 0;
void *g_dialog_system = nullptr;

char g_base_directory[0x105] = {};

void InitDirectories() {
  samp::util::EnsureSampDirectories(g_base_directory, sizeof(g_base_directory));
}

void TakeScreenshot() {
  samp::ui::HookGameWindow();
}

void *StartGame() {
  return samp::game::StartNewGame();
}

void HideHardwareCursor(void *device) {
  if (!device) {
    return;
  }
  using ShowCursorFunction = BOOL(__stdcall *)(void *, BOOL);
  void **vtable = *reinterpret_cast<void ***>(device);
  reinterpret_cast<ShowCursorFunction>(vtable[12])(device, FALSE);
}

void *CreateFont(void *device) {
  return samp::ui::CreateDeviceFont(&samp::ui::Direct3DRenderDevice(device));
}

void *CreateChat(void *device, void *font, const char *log_path) {
  return samp::ui::CreateChatWindow(&samp::ui::Direct3DRenderDevice(device),
                                    static_cast<samp::ui::Font *>(font), log_path);
}

void *CreateSecondChat(void *device) {
  return samp::ui::CreateSecondChat(device);
}

void InitDialogSystem() {
  g_dialog_system = samp::ui::CreateDialogSystem(g_game_device);
}

void *CreateRenderer(void *device) {
  return samp::ui::CreateRenderer(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateSprite(void *device) {
  return samp::ui::CreateSprite(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)),
                                static_cast<unsigned>(g_sprite_arg));
}

void *CreateLineRenderer(void *device) {
  return samp::ui::CreateLineRenderer(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateScoreboard(void *device) {
  return samp::ui::CreateScoreboard(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateDialogState(void *device) {
  return samp::ui::CreateDialogState(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateVehicleDialog(void *device) {
  return samp::ui::CreateVehicleSelect(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateProgressScreen(void *device) {
  return samp::ui::CreateProgressScreen(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateVehicleSelectSmall(void *device) {
  return samp::ui::CreateVehicleSelectSmall(
      static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateTimer(void *device) {
  return samp::ui::CreateFrameTimer(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateTextLabels() {
  return samp::ui::CreateTextLabelPool();
}

void *CreateObjectEditor(void *device) {
  return samp::ui::CreateObjectEditor(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreatePickup() {
  return samp::ui::CreatePickupInfo();
}

void *CreateD3DManager(void *device) {
  return samp::ui::CreateD3DManager(static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void *CreateSpriteManager(void *device) {
  return samp::ui::CreateSpriteManager(
      static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
}

void InitDialogPanels() {
}

void RegisterCommands() {
  samp::ui::RegisterSampCommands(static_cast<samp::ui::ChatWindow *>(g_chat));
}

void ShutdownGame() {
  using GameRoutine = void(__cdecl *)();
  reinterpret_cast<GameRoutine>(0x4399D0)();
  reinterpret_cast<GameRoutine>(0x439940)();
  samp::game::WriteByte(reinterpret_cast<void *>(0x55A070), 0xC3);
}

void SetGravity(void *controller, int gravity) {
  samp::client::ClientController *main = samp::client::MainController();
  if (controller) {
    main = static_cast<samp::client::ClientController *>(controller);
  }
  if (main) {
    samp::client::SetControllerGravity(*main, gravity != 0);
  }
}

void SetFrameLimiter(void *controller, int limit) {
  g_frame_limiter = limit;
  if (controller) {
    samp::client::SetControllerFrameLimit(*static_cast<samp::client::ClientController *>(controller),
                                          static_cast<unsigned>(limit));
  }
  samp::game::SetEngineTickRate(limit);
}

void InitAudio(void *flag) {
  (void)flag;
  samp::audio::InitAudioStreams();
}

int ReadSettingInt(const char *name) {
  return GetSessionSettingInt(name);
}

void WriteSettingInt(const char *name, int value) {
  SetSessionSettingInt(name, value);
}

bool HasSetting(const char *name) {
  return HasSessionSetting(name);
}

}  // namespace

bool SessionReady() {
  return g_session_ready;
}

bool InitializeSessionOnce() {
  return InitializeSessionOnce(g_sprite_arg);
}

bool InitializeSessionOnce(int sprite_arg) {
  if (g_session_ready) {
    return true;
  }
  g_sprite_arg = sprite_arg;
  InitDirectories();
  samp::util::WriteLogLine("directories ready");
  char config_path[MAX_PATH] = {};
  char log_path[MAX_PATH] = {};
  sprintf_s(config_path, "%s\\sa-mp.cfg", g_base_directory);
  sprintf_s(log_path, "%s\\chatlog.txt", g_base_directory);
  LoadSessionSettings(config_path);
  ::timeBeginPeriod(1);
  samp::game::SetAntiCheatFlag();
  TakeScreenshot();
  g_game_device = StartGame();
  HideHardwareCursor(g_game_device);
  g_font = CreateFont(g_game_device);
  g_chat = CreateChat(g_game_device, g_font, log_path);
  g_second_chat = CreateSecondChat(g_game_device);
  g_text_buffer_a = static_cast<unsigned char *>(std::calloc(1, 0x186A1));
  g_text_buffer_b = static_cast<unsigned char *>(std::calloc(1, 0x186A1));
  samp::util::WriteLogLine("chat buffers ready");
  if (ReadSettingInt("timestamp")) {
    samp::ui::SetChatTimestamp(static_cast<samp::ui::ChatWindow *>(g_chat), true);
  }
  int page_size = ReadSettingInt("pagesize");
  if (page_size > 0) {
    samp::ui::SetChatPageSize(static_cast<samp::ui::ChatWindow *>(g_chat), page_size);
  }
  if (ReadSettingInt("nohudscalefix") == 1) {
    samp::game::SetWidescreenFix(false);
  }
  InitDialogSystem();
  if (samp::client::CurrentLaunchParams().connect_fallback) {
    CreateRenderer(g_game_device);
    CreateSprite(g_game_device);
    CreateLineRenderer(g_game_device);
    CreateScoreboard(g_game_device);
    CreateDialogState(g_game_device);
    CreateVehicleDialog(g_game_device);
    CreateProgressScreen(g_game_device);
    CreateTimer(g_game_device);
    CreateVehicleSelectSmall(g_game_device);
    CreateTextLabels();
  }
  CreateObjectEditor(g_game_device);
  CreatePickup();
  CreateD3DManager(g_game_device);
  CreateSpriteManager(g_game_device);
  InitDialogPanels();
  samp::util::WriteLogLine("ui subsystems ready");
  RegisterCommands();
  samp::ui::InstallDisplayModeHook();
  ShutdownGame();
  SetGravity(nullptr, 0);
  if (samp::client::CurrentLaunchParams().connect_primary &&
      samp::client::CurrentLaunchParams().debug_script[0]) {
    samp::scripting::LoadDebugScript(samp::client::CurrentLaunchParams().debug_script);
  }
  int fps_limit = ReadSettingInt("fpslimit");
  if (!fps_limit) {
    WriteSettingInt("fpslimit", 48);
    fps_limit = 48;
  }
  if (fps_limit >= 20 && fps_limit <= 90) {
    SetFrameLimiter(MainController(), fps_limit);
  }
  if (!HasSetting("multicore")) {
    WriteSettingInt("multicore", 1);
  }
  if (!ReadSettingInt("multicore")) {
    ::SetProcessAffinityMask(::GetCurrentProcess(), 1);
  }
  unsigned char *audio_flag = samp::audio::AudioFlag();
  InitAudio(audio_flag);
  samp::util::WriteLogLine("session ready");
  if (MainController()) {
    SetControllerHeadMovementEnabled(*MainController(), ReadSettingInt("disableheadmove") == 0);
  }
  g_session_ready = true;
  return true;
}
bool ConnectSessionClient() {
  if (HasNetworkClient()) {
    return true;
  }
  if (!samp::client::CurrentLaunchParams().connect_fallback) {
    return false;
  }
  const LaunchParams &params = CurrentLaunchParams();
  char *end = nullptr;
  unsigned long port = std::strtoul(params.port, &end, 10);
  if (!params.port[0] || !end || *end || port == 0 || port > 65535) {
    return false;
  }
  return CreateNetworkClient(params.host, static_cast<unsigned short>(port),
                             params.player_name, params.password);
}

void ProcessSessionTick() {
  bool was_ready = g_session_ready;
  if (!InitializeSessionOnce()) {
    return;
  }
  if (was_ready) {
    ConnectSessionClient();
  }
}

void *SessionDevice() {
  return g_game_device;
}

}  // namespace samp::client
