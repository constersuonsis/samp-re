#pragma once

namespace samp::ui {

struct Textdraw {
  unsigned char raw[0xA20] = {};
};

struct TextdrawPool {
  void *active[2304] = {};
  Textdraw *items[2304] = {};
};

TextdrawPool *TextdrawsForController(void *controller);
void RenderAllTextdraws(TextdrawPool *pool, bool scoreboard_open);
void RenderTextdrawPreview(Textdraw *draw);
void RenderTextdraw(Textdraw *draw);
void RenderVehiclePreview(Textdraw *draw);

}  // namespace samp::ui
