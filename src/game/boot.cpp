#include "samp/game/boot.h"
#include "samp/game/memory.h"
#include "samp/game/version.h"

namespace samp::game {

void *StartNewGame() {
  using NewGame = void *(__cdecl *)();
  if (GameVersionIndex() == 1) {
    return reinterpret_cast<NewGame>(0x7F9D50)();
  }
  if (GameVersionIndex() == 2) {
    return reinterpret_cast<NewGame>(0x7F9D90)();
  }
  return nullptr;
}

void SetEngineTickRate(int limit) {
  (void)limit;
  WriteDword(reinterpret_cast<void *>(0xC1704C), 200);
}

}  // namespace samp::game
