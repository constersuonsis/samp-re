#include "samp/ui/renderer.h"

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

}  // namespace

Renderer *CreateRenderer(unsigned device) {
  Renderer *renderer = new Renderer();
  for (size_t offset = 311; offset <= 339; offset += 4) {
    WriteDword(renderer->raw, offset, 0);
  }
  WriteDword(renderer->raw, 0, 1);
  WriteDword(renderer->raw, 327, device);
  std::memset(renderer->raw + 4, 0, 0x124);
  renderer->raw[296] = 0;
  renderer->raw[297] = 0;
  renderer->raw[298] = 0;
  return renderer;
}

Sprite *CreateSprite(unsigned device, unsigned texture) {
  (void)device;
  Sprite *sprite = new Sprite();
  WriteDword(sprite->raw, 12, texture);
  return sprite;
}

void *CreateLineRenderer(unsigned device) {
  (void)device;
  return ::operator new(0x0C);
}

D3DManager *CreateD3DManager(unsigned device) {
  D3DManager *manager = new D3DManager();
  WriteDword(manager->raw, 32, 0);
  WriteDword(manager->raw, 12, 0);
  WriteDword(manager->raw, 8, 0);
  WriteDword(manager->raw, 4, 0);
  WriteDword(manager->raw, 0, device);
  return manager;
}

SpriteManager *CreateSpriteManager(unsigned device) {
  SpriteManager *manager = new SpriteManager();
  WriteDword(manager->raw, 0, device);
  WriteDword(manager->raw, 4, 0);
  WriteDword(manager->raw, 8, 0);
  return manager;
}

}  // namespace samp::ui
