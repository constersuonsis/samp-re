#include "samp/game/frame.h"
#include "samp/game/anticheat.h"
#include "samp/client/session.h"
#include "samp/client/tick.h"
#include "samp/game/memory.h"
#include "samp/game/memory_patch.h"
#include "samp/ui/textdraw.h"

#include <windows.h>

#include <cstring>

namespace samp::game {
namespace {

bool g_widescreen_fix = true;
unsigned g_saved_view_a = 0;
unsigned g_saved_view_b = 0;
bool g_fov_restore_armed = false;
int g_limiter_armed = 0;
bool g_quit_requested = false;
unsigned g_quit_tick = 0;
unsigned g_render_counter = 0;
bool g_render_flag = false;
bool g_scoreboard_open = false;

using GameFrameCall = unsigned(__cdecl *)(void *);

void RenderTextDraws() {
  samp::ui::TextdrawPool *pool =
      samp::ui::TextdrawsForController(samp::client::SessionDevice());
  samp::ui::RenderAllTextdraws(pool, g_scoreboard_open);
}

void DestroyClient() {
}

}  // namespace

void FixWidescreen() {
  if (!g_widescreen_fix) {
    return;
  }
  g_saved_view_a = *reinterpret_cast<unsigned *>(0x859520);
  g_saved_view_b = *reinterpret_cast<unsigned *>(0x859524);
  float width = static_cast<float>(*reinterpret_cast<volatile int *>(0xC17044));
  float height = static_cast<float>(*reinterpret_cast<volatile int *>(0xC17048));
  if (*reinterpret_cast<volatile int *>(0xC17044) <= 0 ||
      *reinterpret_cast<volatile int *>(0xC17048) <= 0) {
    return;
  }
  if (width / height < 1.6f) {
    WriteDword(reinterpret_cast<void *>(0x866B74), 1117257728);
    WriteDword(reinterpret_cast<void *>(0x866B78), 1119617024);
  } else {
    WriteDword(reinterpret_cast<void *>(0x866B74), 1118044160);
    WriteDword(reinterpret_cast<void *>(0x866B78), 1119879168);
    float aspect = width / height;
    float view = 0.0022199999f / aspect;
    std::memcpy(reinterpret_cast<void *>(0x859520), &view, sizeof(view));
    WriteDword(reinterpret_cast<void *>(0x859524), 991860957);
    g_fov_restore_armed = true;
  }
}

void SetWidescreenFix(bool enabled) {
  g_widescreen_fix = enabled;
}

void RenderFrame(void *scene) {
  FixWidescreen();
  using GameRender = void(__cdecl *)(void *);
  reinterpret_cast<GameRender>(0x58A330)(scene);
  if (g_fov_restore_armed) {
    WriteDword(reinterpret_cast<void *>(0x859520), g_saved_view_a);
    WriteDword(reinterpret_cast<void *>(0x859524), g_saved_view_b);
    WriteDword(reinterpret_cast<void *>(0x866B74), 1117257728);
    WriteDword(reinterpret_cast<void *>(0x866B78), 1119617024);
    g_fov_restore_armed = false;
  }
}

void ArmFovRestore(unsigned view_a, unsigned view_b) {
  g_saved_view_a = view_a;
  g_saved_view_b = view_b;
  g_fov_restore_armed = true;
}

void RequestQuit() {
  g_quit_requested = true;
  g_quit_tick = ::GetTickCount();
}

void RenderFrame2GateTextdraws(bool paused) {
  if (paused) {
    return;
  }
  if (!samp::client::HasClient()) {
    return;
  }
  if (!samp::client::ClientOverlayFlag()) {
    return;
  }
  RenderTextDraws();
}

bool GamePaused() {
  return *reinterpret_cast<volatile unsigned char *>(0xBA67A4) != 0;
}

void NoteRenderCheck() {
  ++g_render_counter;
}

unsigned long long RenderFrame2() {
  unsigned entry_edx = 0;
  unsigned entry_ecx = 0;
  unsigned entry_ebx = 0;
  unsigned entry_edi = 0;
  __asm {
    mov entry_edx, edx
    mov entry_ecx, ecx
    mov entry_ebx, ebx
    mov entry_edi, edi
  }
  bool paused = (entry_ebx & 0xFF) != 0;
  ValidateMemory();
  samp::client::ProcessClientTick(paused, static_cast<int>(entry_edi));
  unsigned frame_token = static_cast<unsigned>(g_limiter_armed);
  if (g_limiter_armed ||
      (frame_token = reinterpret_cast<GameFrameCall>(0x53E230)(
                         reinterpret_cast<void *>(entry_ecx)),
       g_limiter_armed)) {
    g_limiter_armed = 0;
  }
  ValidateMemory();
  unsigned long long frame_id =
      (static_cast<unsigned long long>(entry_edx) << 32) | frame_token;
  void *controller = samp::client::SessionDevice();
  if (controller && !GamePaused()) {
    RenderFrame2GateTextdraws(paused);
  }
  if (g_quit_requested) {
    if (::GetTickCount() - g_quit_tick > 0x3E8) {
      samp::client::DestroyClient();
      ::ExitProcess(0);
    }
    return frame_id;
  }
  if (controller) {
    ProcessMemoryPatch();
  }
  if (g_render_counter > 0xA) {
    g_render_flag = true;
  }
  return frame_id;
}

}  // namespace samp::game
