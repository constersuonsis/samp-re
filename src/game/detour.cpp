#include "samp/game/detour.h"
#include "samp/game/length_decoder.h"
#include "samp/game/memory.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::game {
namespace {

void *g_show_cursor_hook = nullptr;

bool IsTerminalOpcode(unsigned char code, unsigned char next) {
  if (code == 0xE9 || code == 0xE0 || code == 0xC2 || code == 0xC3) {
    return true;
  }
  return code == 0xFF && next == 0x25;
}

void EmitJump(unsigned char *at, const void *target) {
  at[0] = 0xE9;
  unsigned relative =
      static_cast<unsigned>(reinterpret_cast<uintptr_t>(target) - (reinterpret_cast<uintptr_t>(at) + 5));
  std::memcpy(at + 1, &relative, sizeof(relative));
}

}  // namespace

void *ResolveImport(const char *library, const char *proc) {
  HMODULE module = ::LoadLibraryA(library);
  if (!module) {
    return nullptr;
  }
  return reinterpret_cast<void *>(::GetProcAddress(module, proc));
}

unsigned char *NormalizeTarget(unsigned char *address, bool follow_jump) {
  if (!address) {
    return nullptr;
  }
  if (address[0] == 0xFF && address[1] == 0x25) {
    unsigned char **slot = nullptr;
    std::memcpy(&slot, address + 2, sizeof(slot));
    return slot ? *slot : nullptr;
  }
  if (address[0] == 0xE9 && follow_jump) {
    int relative = 0;
    std::memcpy(&relative, address + 1, sizeof(relative));
    return address + relative + 5;
  }
  return address;
}

unsigned MeasurePrologue(unsigned char *address) {
  if (!address) {
    return 0;
  }
  LengthDecoder decoder;
  unsigned total = 0;
  while (total < 5) {
    unsigned char code = address[total];
    unsigned char next = address[total + 1];
    if (IsTerminalOpcode(code, next)) {
      break;
    }
    unsigned step = DecodeInstruction(decoder, address + total);
    if (!step) {
      return 0;
    }
    total += step;
  }
  if (total < 5 || total > 26) {
    return 0;
  }
  return total;
}

void *InstallInlineHook(void *target, void *detour) {
  unsigned char *normalized_target = NormalizeTarget(static_cast<unsigned char *>(target), true);
  unsigned char *normalized_detour = NormalizeTarget(static_cast<unsigned char *>(detour), false);
  if (!normalized_target || !normalized_detour) {
    return nullptr;
  }
  unsigned length = MeasurePrologue(normalized_target);
  if (!length) {
    return nullptr;
  }
  auto *record = static_cast<HookRecord *>(
      ::VirtualAlloc(nullptr, sizeof(HookRecord), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
  if (!record) {
    return nullptr;
  }
  DWORD old_protection = 0;
  if (!::VirtualProtect(normalized_target, length, PAGE_EXECUTE_READWRITE, &old_protection)) {
    ::VirtualFree(record, 0, MEM_RELEASE);
    return nullptr;
  }
  std::memcpy(record->saved, normalized_target, length);
  std::memcpy(record->trampoline, normalized_target, length);
  EmitJump(record->trampoline + length, normalized_target + length);
  record->prologue_length = static_cast<unsigned char>(length);
  EmitJump(normalized_target, normalized_detour);
  for (unsigned i = 5; i < length; ++i) {
    normalized_target[i] = 0x90;
  }
  DWORD restored_protection = 0;
  ::VirtualProtect(normalized_target, length, old_protection, &restored_protection);
  ::FlushInstructionCache(::GetCurrentProcess(), record->trampoline, length + 5);
  ::FlushInstructionCache(::GetCurrentProcess(), normalized_target, length);
  return record;
}

int __stdcall CompareShowCursorArg(int visible) {
  return (visible != 0) - 1;
}

bool HookShowCursor() {
  void *show_cursor = ResolveImport("user32.dll", "ShowCursor");
  if (!show_cursor) {
    return false;
  }
  g_show_cursor_hook = InstallInlineHook(show_cursor, reinterpret_cast<void *>(CompareShowCursorArg));
  return g_show_cursor_hook != nullptr;
}

}  // namespace samp::game
