#include "samp/ui/font.h"
#include "samp/client/settings.h"
#include "samp/ui/render_device.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace samp::ui {
namespace {

int ScreenWidth() {
  return *reinterpret_cast<volatile int *>(0xC17044);
}

int ResolveFontSize() {
  int width = ScreenWidth();
  int base = 14;
  if (width >= 1024) {
    base = 16;
    if (width >= 1400) {
      base = 18 + 2 * (width >= 1600 ? 1 : 0);
    }
  }
  return base + 2 * samp::client::GetSettingInt("fontsize", 0);
}

int ResolveFontWeight() {
  int weight = samp::client::GetSettingInt("fontweight", 0);
  if (!weight) {
    return 700;
  }
  return weight != 1 ? 700 : 400;
}

const char *ResolveFontFace() {
  const char *face = samp::client::GetSettingString("fontface");
  if (face && face[0]) {
    return face;
  }
  return "Arial";
}

void ReleaseSlot(RenderDevice *device, Font *font, size_t slot) {
  unsigned handle = 0;
  std::memcpy(&handle, font->raw + slot * 4, sizeof(handle));
  if (handle) {
    device->ReleaseFont(reinterpret_cast<void *>(handle));
    unsigned empty = 0;
    std::memcpy(font->raw + slot * 4, &empty, sizeof(empty));
  }
}

void StoreSlot(Font *font, size_t slot, void *handle) {
  unsigned value = static_cast<unsigned>(reinterpret_cast<uintptr_t>(handle));
  std::memcpy(font->raw + slot * 4, &value, sizeof(value));
}

}  // namespace

Font *CreateDeviceFont(RenderDevice *render) {
  if (!render) {
    render = &NullRenderDevice();
  }
  Font *font = new Font();
  StoreSlot(font, 6, render);
  int size = ResolveFontSize();
  int weight = ResolveFontWeight();
  const char *face = ResolveFontFace();
  for (size_t slot = 0; slot < 6; ++slot) {
    ReleaseSlot(render, font, slot);
  }
  StoreSlot(font, 0, render->CreateFontFace(size, weight, face));
  StoreSlot(font, 2, render->CreateFontFace(size, weight, face));
  StoreSlot(font, 1, render->CreateFontFace(size - 2, weight, face));
  StoreSlot(font, 3, render->CreateFontFace(size, weight, face));
  StoreSlot(font, 5, render->CreateSprite());
  unsigned char *buffer = static_cast<unsigned char *>(std::calloc(1, 0x186A1));
  StoreSlot(font, 7, buffer);
  StoreSlot(font, 4, render->CreateFontFace(38, 700, "Arial"));
  TextBounds bounds;
  unsigned handle = 0;
  std::memcpy(&handle, font->raw, sizeof(handle));
  if (render->MeasureText(reinterpret_cast<void *>(handle), "Y", bounds)) {
    StoreSlot(font, 8, reinterpret_cast<void *>(bounds.bottom - bounds.top));
  }
  std::memcpy(&handle, font->raw + 4, sizeof(handle));
  if (render->MeasureText(reinterpret_cast<void *>(handle), "Y", bounds)) {
    StoreSlot(font, 9, reinterpret_cast<void *>(bounds.bottom - bounds.top));
  }
  return font;
}

void DestroyFont(Font *font) {
  delete font;
}

}  // namespace samp::ui
