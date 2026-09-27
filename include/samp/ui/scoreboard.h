#pragma once

namespace samp::ui {

struct Scoreboard {
  unsigned char raw[0x44] = {};
};

struct ProgressScreen {
  unsigned char raw[0x260] = {};
};

Scoreboard *CreateScoreboard(unsigned device);
ProgressScreen *CreateProgressScreen(unsigned device);
void CenterScoreboard(Scoreboard *board);

}  // namespace samp::ui
