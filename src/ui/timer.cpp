#include "samp/ui/timer.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

}  // namespace

FrameTimer *CreateFrameTimer(unsigned device) {
  FrameTimer *timer = new FrameTimer();
  WriteDword(timer->raw, 8, ::GetTickCount());
  WriteDword(timer->raw, 4, 0);
  WriteDword(timer->raw, 0, 0);
  WriteDword(timer->raw, 16, 0);
  WriteDword(timer->raw, 12, 0);
  WriteDword(timer->raw, 20, device);
  return timer;
}

}  // namespace samp::ui
