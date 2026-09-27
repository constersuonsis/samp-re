#pragma once

namespace samp::ui {

struct DisplayMode {
  unsigned width = 0;
  unsigned height = 0;
  unsigned refresh_rate = 0;
  unsigned format = 0;
};

struct SurfaceDesc {
  unsigned format = 0;
  unsigned type = 0;
  unsigned usage = 0;
  unsigned pool = 0;
  unsigned multisample_type = 0;
  unsigned multisample_quality = 0;
  unsigned width = 0;
  unsigned height = 0;
};

struct TextBounds {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
};

class RenderDevice {
 public:
  virtual ~RenderDevice() = default;
  virtual void *CreateSprite() = 0;
  virtual void ReleaseSurface(void *surface) = 0;
  virtual bool DisplayModeInfo(DisplayMode &mode) = 0;
  virtual void *CreateTexture(unsigned width, unsigned height) = 0;
  virtual void *SurfaceLevel(void *texture) = 0;
  virtual bool SurfaceDescription(void *surface, SurfaceDesc &desc) = 0;
  virtual void *CreateRenderTarget(void *surface, const SurfaceDesc &desc) = 0;
  virtual void *CreateFontFace(int height, int weight, const char *face) = 0;
  virtual void *CreateLine() = 0;
  virtual void ReleaseFont(void *font) = 0;
  virtual bool MeasureText(void *font, const char *text, TextBounds &bounds) = 0;
};

RenderDevice &NullRenderDevice();
RenderDevice &Direct3DRenderDevice(void *device);

}  // namespace samp::ui
