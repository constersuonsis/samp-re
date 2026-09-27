#pragma once

namespace samp::game {

int AntiCheatCheck(bool entering);
int CrashHandlerProbe(int id);
void SetAntiCheatFlag();
void ValidateMemory();

}  // namespace samp::game
