#include "samp/ui/game_window.h"

#include <windows.h>

namespace samp::ui {
namespace {

WNDPROC g_previous_proc = nullptr;

}  // namespace

LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM param, LPARAM reserved) {
  if (!g_previous_proc) {
    return ::DefWindowProcA(window, message, param, reserved);
  }
  return ::CallWindowProcA(g_previous_proc, window, message, param, reserved);
}

bool HookGameWindow() {
  HWND window = *reinterpret_cast<HWND *>(0xC97C1C);
  if (!window) {
    return false;
  }
  unsigned style = ::GetClassLongA(window, GCL_STYLE);
  ::SetClassLongA(window, GCL_STYLE, style | CS_OWNDC);
  WNDPROC current =
      reinterpret_cast<WNDPROC>(::GetWindowLongA(window, GWL_WNDPROC));
  if (current != GameWindowProc) {
    g_previous_proc = current;
    ::SetWindowLongA(window, GWL_WNDPROC, reinterpret_cast<LONG>(GameWindowProc));
  }
  ::SetWindowTextA(window, "GTA:SA:MP");
  ::IsWindowUnicode(window);
  return true;
}

}  // namespace samp::ui
