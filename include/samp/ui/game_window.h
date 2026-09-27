#pragma once

#include <windows.h>

namespace samp::ui {

bool HookGameWindow();
LRESULT CALLBACK GameWindowProc(HWND window, UINT message, WPARAM param, LPARAM reserved);

}  // namespace samp::ui
