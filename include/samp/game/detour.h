#pragma once

namespace samp::game {

struct HookRecord {
  unsigned char trampoline[32] = {};
  unsigned char saved[26] = {};
  unsigned char prologue_length = 0;
  unsigned char padding = 0;
};

void *ResolveImport(const char *library, const char *proc);
unsigned char *NormalizeTarget(unsigned char *address, bool follow_jump);
unsigned MeasurePrologue(unsigned char *address);
void *InstallInlineHook(void *target, void *detour);
int __stdcall CompareShowCursorArg(int visible);
bool HookShowCursor();

}  // namespace samp::game
