#pragma once

namespace samp::ui {

struct Renderer {
  unsigned char raw[0x157] = {};
};

struct Sprite {
  unsigned char raw[0x20] = {};
};

Renderer *CreateRenderer(unsigned device);
Sprite *CreateSprite(unsigned device, unsigned texture);
void *CreateLineRenderer(unsigned device);

struct D3DManager {
  unsigned char raw[0x24] = {};
};

struct SpriteManager {
  unsigned char raw[0x0C] = {};
};

D3DManager *CreateD3DManager(unsigned device);
SpriteManager *CreateSpriteManager(unsigned device);

}  // namespace samp::ui
