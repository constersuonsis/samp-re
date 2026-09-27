#include "samp/ui/display.h"

#include <cstddef>

namespace samp::ui {
namespace {

using ModeHandler = int(__cdecl *)(int);
using SetRenderState = long(__stdcall *)(void *, unsigned, unsigned);

ModeHandler g_previous_handler = nullptr;

void SetTextureFilter(int enabled) {
  void *device = *reinterpret_cast<void **>(0x1026E888);
  if (!device) {
    return;
  }
  const unsigned filter = *reinterpret_cast<volatile unsigned *>(0x1026E928)
                              ? 0U
                              : static_cast<unsigned>(enabled);
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
      *reinterpret_cast<volatile int *>(0x1026E924) = 1;
    } else {
      SetTextureFilter(0);
      *reinterpret_cast<volatile int *>(0x1026E924) = 0;
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
