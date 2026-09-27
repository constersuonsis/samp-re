#pragma once

namespace samp::ui {

struct FrameTimer {
  unsigned char raw[0x18] = {};
};

FrameTimer *CreateFrameTimer(unsigned device);

}  // namespace samp::ui
