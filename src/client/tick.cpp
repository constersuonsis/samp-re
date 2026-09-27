#include "samp/client/tick.h"
#include "samp/audio/audio.h"
#include "samp/client/controller.h"
#include "samp/client/network_client.h"
#include "samp/client/session.h"
#include "samp/client/spawn.h"
#include "samp/game/anticheat.h"
#include "samp/util/precise_time.h"

#include <windows.h>

#include <cstdlib>
#include <d3d9.h>
namespace samp::client {
namespace {

bool g_fog_override = false;
bool g_overlay_enabled = false;

void ApplyDeviceRenderStates(void *device, bool full_detail) {
  if (!device) {
    return;
  }
  using SetState = void(__stdcall *)(void *, int, int);
  unsigned *vtable = *reinterpret_cast<unsigned **>(device);
  auto call = reinterpret_cast<SetState>(vtable[57]);
  call(device, D3DRS_FOGENABLE, full_detail ? 1 : 0);
  call(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
  call(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
}
bool g_time_synced = false;
unsigned char g_time_hour = 0;
unsigned char g_time_minute = 0;
void ProcessIncomingPackets() {
  PumpNetworkClient();
}

void SetTimeOfDay(unsigned char hour, unsigned char minute) {
  g_time_hour = hour;
  g_time_minute = minute;
  *reinterpret_cast<volatile unsigned char *>(0xB70152) = minute;
  *reinterpret_cast<volatile unsigned char *>(0xB70153) = hour;
}

void PreloadEssentials() {
}

void HideScoreboard() {
}

void UpdateWorld() {
}

void CheckDownloads() {
}

void HandleClassSelection() {
}

void TryConnect() {
  TryConnectNetworkClient();
}

void ClientTick(bool paused) {
  (void)paused;
  samp::util::PreciseDeltaTime();
  int check_start = samp::game::AntiCheatCheck(true);
  ProcessIncomingPackets();
  if (g_time_synced) {
    SetTimeOfDay(g_time_hour, g_time_minute);
  }
  PreloadEssentials();
  if (NetworkClientState() == 5) {
    HideScoreboard();
    UpdateWorld();
    CheckDownloads();
    HandleClassSelection();
  } else {
    DoSpawn();
  }
  if (NetworkClientState() == 1) {
    TryConnect();
  }
  int check_end = samp::game::AntiCheatCheck(false);
  if (check_end - check_start < 1) {
    if (samp::game::CrashHandlerProbe(69) != 69) {
      ::exit(0);
    }
  }
}

int InvokeStatusOpA() {
  return 0;
}

int InvokeStatusOpB() {
  return 0;
}

int RunStatusMachine() {
  ClientController *controller = MainController();
  if (!controller) {
    return 0;
  }
  if (!controller->status) {
    return 0;
  }
  unsigned *status = static_cast<unsigned *>(controller->status);
  int result = static_cast<int>(status[0]);
  if (status[0]) {
    if (result == 1) {
      result = InvokeStatusOpA();
      if (result) {
        result = InvokeStatusOpB();
        status[0] = 2;
      }
    }
  }
  return result;
}

}  // namespace

int ProcessClientTick(bool paused, int init_arg) {
  bool was_ready = SessionReady();
  InitializeSessionOnce(init_arg);
  if (was_ready) {
    ConnectSessionClient();
  }
  ApplyDeviceRenderStates(SessionDevice(), !g_fog_override);
  if (HasNetworkClient()) {
    ClientTick(paused);
  }
  if (samp::audio::AudioFlag()) {
    samp::audio::StopAudioStreams();
  }
  return RunStatusMachine();
}

void SyncTimeOfDay(unsigned char hour, unsigned char minute) {
  g_time_synced = true;
  g_time_hour = hour;
  g_time_minute = minute;
}

bool HasClient() {
  return HasNetworkClient();
}

bool ClientOverlayFlag() {
  return g_overlay_enabled;
}

void DestroyClient() {
  DestroyNetworkClient();
  g_time_synced = false;
}

}  // namespace samp::client
