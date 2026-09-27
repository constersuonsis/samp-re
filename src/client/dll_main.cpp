#include <windows.h>

#include "samp/client/startup.h"

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
  (void)reserved;
  if (reason == DLL_PROCESS_ATTACH) {
    return samp::client::HandleAttach(module) ? TRUE : FALSE;
  }
  if (reason == DLL_PROCESS_DETACH) {
    samp::client::HandleDetach();
  }
  return TRUE;
}
