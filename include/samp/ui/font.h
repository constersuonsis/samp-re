#pragma once

namespace samp::ui {

class RenderDevice;

struct Font {
  unsigned char raw[0x28] = {};
};

Font *CreateDeviceFont(RenderDevice *device);
void DestroyFont(Font *font);

}  // namespace samp::ui
