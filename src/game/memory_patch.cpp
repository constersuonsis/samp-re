#include "samp/game/memory_patch.h"
#include "samp/client/session.h"
#include "samp/game/memory.h"
#include "samp/game/version.h"

namespace samp::game {
namespace {

const unsigned char kCallSitePatch[4] = {0xE8, 0x46, 0xF3, 0xFE};
const unsigned char kCallSiteTail = 0xFF;
const unsigned char kHookV1[4] = {0xE8, 0xB4, 0x7A, 0x20};
const unsigned char kHookV1Tail = 0x00;
const unsigned char kHookV2[4] = {0xE8, 0x04, 0x7B, 0x20};
const unsigned char kHookV2Tail = 0x00;

int g_restore_countdown = 0;
bool g_patch_armed = false;

void RestoreModelPointers() {
}

using GameCallback = void(__cdecl *)();

void RunGameCallback(unsigned address) {
  reinterpret_cast<GameCallback>(address)();
}

}  // namespace

bool MemoryPatchArmed() {
  return g_patch_armed;
}

void ArmMemoryPatch(int frames) {
  g_restore_countdown = frames;
  g_patch_armed = true;
}

void ProcessMemoryPatch() {
  if (g_restore_countdown) {
    --g_restore_countdown;
    return;
  }
  if (!g_patch_armed) {
    return;
  }
  g_patch_armed = false;
  WriteBytes(reinterpret_cast<void *>(0x541DF5), kCallSitePatch, sizeof(kCallSitePatch));
  WriteByte(reinterpret_cast<void *>(0x541DF9), kCallSiteTail);
  if (GameVersionIndex() == 1) {
    WriteBytes(reinterpret_cast<void *>(0x53F417), kHookV1, sizeof(kHookV1));
    WriteByte(reinterpret_cast<void *>(0x53F41B), kHookV1Tail);
  } else {
    WriteBytes(reinterpret_cast<void *>(0x53F417), kHookV2, sizeof(kHookV2));
    WriteByte(reinterpret_cast<void *>(0x53F41B), kHookV2Tail);
  }
  RestoreModelPointers();
  WriteByte(reinterpret_cast<void *>(0x53F41F), 0x85);
  WriteByte(reinterpret_cast<void *>(0x53F420), 0xC0);
  WriteByte(reinterpret_cast<void *>(0x53F421), 0x0F);
  WriteByte(reinterpret_cast<void *>(0x53F422), 0x8C);
  WriteDword(reinterpret_cast<void *>(0xB73424), 0);
  WriteDword(reinterpret_cast<void *>(0xB73428), 0);
  RunGameCallback(0x541BD0);
  RunGameCallback(0x541DD0);
  WriteDword(reinterpret_cast<void *>(0xB73424), 0);
  WriteDword(reinterpret_cast<void *>(0xB73428), 0);
  RunGameCallback(0x541BD0);
  WriteByte(reinterpret_cast<void *>(0x6194A0), 0xE9);
  using DeviceReset = void(__stdcall *)(void *, unsigned);
  void *device = samp::client::SessionDevice();
  if (device) {
    unsigned *vtable = *reinterpret_cast<unsigned **>(device);
    reinterpret_cast<DeviceReset>(vtable[12])(device, 0);
  }
}

}  // namespace samp::game
