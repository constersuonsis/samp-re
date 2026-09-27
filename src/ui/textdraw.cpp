#include "samp/ui/textdraw.h"
#include "samp/game/vehicle_preview.h"
#include "samp/ui/display.h"

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

float ReadFloat(const unsigned char *base, size_t offset) {
  float value = 0.0f;
  std::memcpy(&value, base + offset, sizeof(value));
  return value;
}

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

float ScreenScaleX() {
  return static_cast<float>(*reinterpret_cast<volatile int *>(0xC17044)) *
         *reinterpret_cast<volatile float *>(0x859520);
}

float ScreenScaleY() {
  return static_cast<float>(*reinterpret_cast<volatile int *>(0xC17048)) *
         *reinterpret_cast<volatile float *>(0x859524);
}

void DrawPreviewTexture(unsigned texture, const float *rect, size_t style_offset,
                        const unsigned char *draw) {
  if (!texture) {
    return;
  }
  samp::ui::DisplayModeHook(9, 2);
  using GameDraw = void(__cdecl *)(unsigned, const float *, const unsigned char *);
  reinterpret_cast<GameDraw>(0x728350)(texture, rect, draw + style_offset);
}

void SetupVehiclePreview() {
  samp::game::PreviewGetRotation();
  samp::game::PreviewDelete();
  samp::game::PreviewSetRotation();
  samp::game::PreviewSetSpeed();
}

void TeardownVehiclePreview() {
  samp::game::PreviewGetModel();
  samp::game::PreviewSetDoor();
  samp::game::PreviewGetHealth();
  samp::game::PreviewSetColor();
  samp::game::PreviewSetHealth();
  samp::game::PreviewSetPosition();
}

void PlacePreviewVehicle(float x, float y, const char *model) {
  samp::game::PreviewGetPosition();
  samp::game::PreviewGetSpeed();
  samp::game::PreviewGetMaxCount();
  samp::game::PreviewCreate(x, y, model);
  samp::game::PreviewGetPosition();
}

}  // namespace

TextdrawPool *TextdrawsForController(void *controller) {
  if (!controller) {
    return nullptr;
  }
  unsigned char *base = static_cast<unsigned char *>(controller);
  unsigned slot = 0;
  std::memcpy(&slot, base + 8, sizeof(slot));
  if (!slot) {
    TextdrawPool *pool = new TextdrawPool();
    std::memcpy(base + 8, &pool, sizeof(pool));
    return pool;
  }
  return reinterpret_cast<TextdrawPool *>(slot);
}

void RenderAllTextdraws(TextdrawPool *pool, bool scoreboard_open) {
  if (!pool || scoreboard_open) {
    return;
  }
  for (int i = 0; i < 2304; ++i) {
    if (pool->active[i] && pool->items[i]) {
      RenderTextdrawPreview(pool->items[i]);
    }
  }
}

void RenderTextdrawPreview(Textdraw *draw) {
  if (!draw) {
    return;
  }
  int shaped = 0;
  std::memcpy(&shaped, draw->raw + 2467, sizeof(shaped));
  if (shaped == -1) {
    RenderVehiclePreview(draw);
  } else {
    RenderTextdraw(draw);
  }
}

void RenderTextdraw(Textdraw *draw) {
  float scale_x = ScreenScaleX();
  float scale_y = ScreenScaleY();
  float x = ReadFloat(draw->raw, 2443);
  float y = ReadFloat(draw->raw, 2447);
  float w = ReadFloat(draw->raw, 2418);
  float h = ReadFloat(draw->raw, 2422);
  float rect[4] = {x * scale_x, y * scale_y, (x + w) * scale_x, (y + h) * scale_y};
  int shaped = 0;
  std::memcpy(&shaped, draw->raw + 2467, sizeof(shaped));
  if (draw->raw[2513]) {
    DrawPreviewTexture(0, rect, 2514, draw->raw);
  } else {
    DrawPreviewTexture(0, rect, 2411, draw->raw);
  }
  (void)shaped;
  unsigned left = static_cast<unsigned>(rect[0]);
  unsigned right = static_cast<unsigned>(rect[0] + scale_x * w);
  unsigned top = static_cast<unsigned>(rect[1]);
  unsigned bottom = static_cast<unsigned>(rect[1] + scale_y * h);
  WriteDword(draw->raw, 2497, left);
  WriteDword(draw->raw, 2505, right);
  WriteDword(draw->raw, 2501, top);
  WriteDword(draw->raw, 2509, bottom);
  draw->raw[2495] = 1;
}

void RenderVehiclePreview(Textdraw *draw) {
  if (!draw || !draw->raw[0]) {
    return;
  }
  size_t length = 0;
  while (draw->raw[length] != 0 && length + 1 < 801) {
    draw->raw[801 + length] = draw->raw[length];
    ++length;
  }
  draw->raw[801 + length] = 0;
  float screen_w = static_cast<float>(*reinterpret_cast<volatile int *>(0xC17044));
  float screen_h = static_cast<float>(*reinterpret_cast<volatile int *>(0xC17048));
  float scale_x = ScreenScaleX();
  float scale_y = ScreenScaleY();
  SetupVehiclePreview();
  float w = ReadFloat(draw->raw, 2418) * screen_w * scale_x;
  float h = ReadFloat(draw->raw, 2422) * screen_w * scale_x;
  float x = screen_w - (640.0f - ReadFloat(draw->raw, 2443)) * scale_x;
  float y = screen_h - (448.0f - ReadFloat(draw->raw, 2447)) * scale_y;
  if (draw->raw[2436]) {
    samp::game::PreviewGetPosition();
  } else {
    samp::game::PreviewSetWindow();
  }
  samp::game::PreviewGetSpeed();
  samp::game::PreviewGetMaxCount();
  if (draw->raw[2496]) {
    samp::game::PreviewDestroyAll();
  }
  PlacePreviewVehicle(x, y, reinterpret_cast<const char *>(draw->raw + 801));
  TeardownVehiclePreview();
  unsigned char centered = draw->raw[2416];
  unsigned char right_aligned = draw->raw[2438];
  unsigned left = 0;
  unsigned right = 0;
  if (right_aligned) {
    left = static_cast<unsigned>(x - (w - x));
    right = static_cast<unsigned>(x);
  } else if (centered) {
    left = static_cast<unsigned>(x - h * 0.5f);
    right = static_cast<unsigned>(left + h);
  } else {
    left = static_cast<unsigned>(x);
    right = static_cast<unsigned>(w);
  }
  (void)w;
  WriteDword(draw->raw, 2497, left);
  WriteDword(draw->raw, 2505, right);
  WriteDword(draw->raw, 2501, static_cast<unsigned>(y));
  WriteDword(draw->raw, 2509, static_cast<unsigned>(y + h));
  draw->raw[2495] = 1;
}

}  // namespace samp::ui
