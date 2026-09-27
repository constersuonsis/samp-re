#include "samp/ui/display.h"
#include "samp/client/session.h"

#include <cstddef>

namespace samp::ui {
namespace {

using ModeHandler = int(__cdecl *)(int);
using SetRenderState = long(__stdcall *)(void *, unsigned, unsigned);

ModeHandler g_previous_handler = nullptr;
bool g_texture_filter_disabled = false;
bool g_display_mode_enabled = false;

void SetTextureFilter(int enabled) {
  void *device = samp::client::SessionDevice();
  if (!device) {
    return;
  }
  const unsigned filter = g_texture_filter_disabled ? 0U : static_cast<unsigned>(enabled);
  void **vtable = *reinterpret_cast<void ***>(device);
  auto set_render_state = reinterpret_cast<SetRenderState>(vtable[57]);
  set_render_state(device, 28, filter);
  set_render_state(device, 35, 0);
  set_render_state(device, 140, 3);
}

}  // namespace

int DisplayModeHook(int mode, int enabled) {
  if (mode == 14) {
    if (enabled) {
      SetTextureFilter(1);
      g_display_mode_enabled = true;
    } else {
      SetTextureFilter(0);
      g_display_mode_enabled = false;
    }
  }
  if (!g_previous_handler) {
    return 0;
  }
  return g_previous_handler(mode);
}

void InstallDisplayModeHook() {
  unsigned *slot = *reinterpret_cast<unsigned **>(0xC97B24);
  g_previous_handler = reinterpret_cast<ModeHandler>(slot[8]);
  slot[8] = static_cast<unsigned>(reinterpret_cast<uintptr_t>(&DisplayModeHook));
}

}  // namespace samp::ui
