#include "samp/ui/render_device.h"

#include <windows.h>

namespace samp::ui {
namespace {

HMODULE D3DXModule() {
  static HMODULE module = [] {
    HMODULE loaded = ::GetModuleHandleA("d3dx9_25.dll");
    return loaded ? loaded : ::LoadLibraryA("d3dx9_25.dll");
  }();
  return module;
}

template <typename Function>
Function D3DXFunction(const char *name) {
  HMODULE module = D3DXModule();
  if (!module) {
    return nullptr;
  }
  return reinterpret_cast<Function>(::GetProcAddress(module, name));
}

class Direct3DDevice final : public RenderDevice {
 public:
  void SetDevice(void *device) {
    device_ = device;
  }

  void *CreateSprite() override {
    using CreateSpriteFunction = HRESULT(__stdcall *)(void *, void **);
    const auto create_sprite = D3DXFunction<CreateSpriteFunction>("D3DXCreateSprite");
    void *sprite = nullptr;
    return create_sprite && SUCCEEDED(create_sprite(device_, &sprite)) ? sprite : nullptr;
  }

  void ReleaseSurface(void *surface) override {
    ReleaseResource(surface);
  }

  bool DisplayModeInfo(DisplayMode &mode) override {
    mode = DisplayMode{};
    if (!device_) {
      return false;
    }
    using GetDisplayModeFunction = HRESULT(__stdcall *)(void *, unsigned, DisplayMode *);
    void **vtable = *reinterpret_cast<void ***>(device_);
    return SUCCEEDED(reinterpret_cast<GetDisplayModeFunction>(vtable[8])(device_, 0, &mode));
  }

  void *CreateTexture(unsigned width, unsigned height) override {
    using CreateTextureFunction = HRESULT(__stdcall *)(void *, unsigned, unsigned, unsigned,
                                                        unsigned, int, int, void **);
    const auto create_texture = D3DXFunction<CreateTextureFunction>("D3DXCreateTexture");
    void *texture = nullptr;
    return create_texture &&
                   SUCCEEDED(create_texture(device_, width, height, 1, 1, 21, 0, &texture))
               ? texture
               : nullptr;
  }

  void *SurfaceLevel(void *texture) override {
    if (!texture) {
      return nullptr;
    }
    using GetSurfaceLevelFunction = HRESULT(__stdcall *)(void *, unsigned, void **);
    void **vtable = *reinterpret_cast<void ***>(texture);
    void *surface = nullptr;
    return SUCCEEDED(reinterpret_cast<GetSurfaceLevelFunction>(vtable[18])(texture, 0, &surface))
               ? surface
               : nullptr;
  }

  bool SurfaceDescription(void *surface, SurfaceDesc &desc) override {
    desc = SurfaceDesc{};
    if (!surface) {
      return false;
    }
    using GetDescriptionFunction = HRESULT(__stdcall *)(void *, SurfaceDesc *);
    void **vtable = *reinterpret_cast<void ***>(surface);
    return SUCCEEDED(reinterpret_cast<GetDescriptionFunction>(vtable[12])(surface, &desc));
  }

  void *CreateRenderTarget(void *surface, const SurfaceDesc &desc) override {
    using CreateRenderTargetFunction = HRESULT(__stdcall *)(void *, unsigned, unsigned, int, BOOL,
                                                            int, void **);
    const auto create_render_target =
        D3DXFunction<CreateRenderTargetFunction>("D3DXCreateRenderToSurface");
    if (!surface || !create_render_target) {
      return nullptr;
    }
    void *target = nullptr;
    return SUCCEEDED(create_render_target(device_, desc.width, desc.height,
                                          static_cast<int>(desc.format), TRUE, 80, &target))
               ? target
               : nullptr;
  }

  void *CreateFontFace(int height, int weight, const char *face) override {
    using CreateFontFunction = HRESULT(__stdcall *)(void *, int, unsigned, unsigned, unsigned,
                                                    BOOL, unsigned, unsigned, unsigned, unsigned,
                                                    const char *, void **);
    const auto create_font = D3DXFunction<CreateFontFunction>("D3DXCreateFontA");
    void *font = nullptr;
    return create_font &&
                   SUCCEEDED(create_font(device_, height, 0, static_cast<unsigned>(weight), 1, FALSE,
                                         1, 0, 4, 0, face, &font))
               ? font
               : nullptr;
  }

  void *CreateLine() override {
    using CreateLineFunction = HRESULT(__stdcall *)(void *, void **);
    const auto create_line = D3DXFunction<CreateLineFunction>("D3DXCreateLine");
    void *line = nullptr;
    return create_line && SUCCEEDED(create_line(device_, &line)) ? line : nullptr;
  }

  void ReleaseFont(void *font) override {
    ReleaseResource(font);
  }

  bool MeasureText(void *font, const char *text, TextBounds &bounds) override {
    bounds = TextBounds{};
    if (!font || !text) {
      return false;
    }
    using DrawTextFunction = int(__stdcall *)(void *, void *, const char *, int, RECT *, unsigned,
                                              unsigned);
    void **vtable = *reinterpret_cast<void ***>(font);
    RECT rect = {};
    reinterpret_cast<DrawTextFunction>(vtable[14])(font, nullptr, text, -1, &rect, 0x400,
                                                   0xFF000000);
    bounds.left = rect.left;
    bounds.top = rect.top;
    bounds.right = rect.right;
    bounds.bottom = rect.bottom;
    return true;
  }

 private:
  static void ReleaseResource(void *resource) {
    if (!resource) {
      return;
    }
    using ReleaseFunction = unsigned long(__stdcall *)(void *);
    void **vtable = *reinterpret_cast<void ***>(resource);
    reinterpret_cast<ReleaseFunction>(vtable[2])(resource);
  }

  void *device_ = nullptr;
};

class NullDevice : public RenderDevice {
 public:
  void *CreateSprite() override {
    return nullptr;
  }
  void ReleaseSurface(void *surface) override {
    (void)surface;
  }
  bool DisplayModeInfo(DisplayMode &mode) override {
    mode = DisplayMode{};
    mode.width = 1024;
    mode.height = 512;
    return true;
  }
  void *CreateTexture(unsigned width, unsigned height) override {
    (void)width;
    (void)height;
    return nullptr;
  }
  void *SurfaceLevel(void *texture) override {
    (void)texture;
    return nullptr;
  }
  bool SurfaceDescription(void *surface, SurfaceDesc &desc) override {
    (void)surface;
    desc = SurfaceDesc{};
    return false;
  }
  void *CreateRenderTarget(void *surface, const SurfaceDesc &desc) override {
    (void)surface;
    (void)desc;
    return nullptr;
  }
  void *CreateFontFace(int height, int weight, const char *face) override {
    (void)height;
    (void)weight;
    (void)face;
    return nullptr;
  }
  void *CreateLine() override {
    return nullptr;
  }
  void ReleaseFont(void *font) override {
    (void)font;
  }
  bool MeasureText(void *font, const char *text, TextBounds &bounds) override {
    (void)font;
    (void)text;
    bounds = TextBounds{};
    bounds.bottom = 16;
    bounds.right = 8;
    return true;
  }
};

NullDevice g_null_device;
Direct3DDevice g_direct3d_device;

}  // namespace

RenderDevice &NullRenderDevice() {
  return g_null_device;
}

RenderDevice &Direct3DRenderDevice(void *device) {
  if (!device) {
    return g_null_device;
  }
  g_direct3d_device.SetDevice(device);
  return g_direct3d_device;
}

}  // namespace samp::ui
