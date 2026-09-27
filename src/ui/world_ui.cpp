#include "samp/ui/world_ui.h"
#include "samp/ui/render_device.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

}  // namespace

TextLabelPool *CreateTextLabelPool() {
  return new TextLabelPool();
}

ObjectEditor *CreateObjectEditor(unsigned device) {
  RenderDevice *render = &NullRenderDevice();
  ObjectEditor *editor = new ObjectEditor();
  WriteDword(editor->raw, 259, device);
  WriteDword(editor->raw, 124, 0);
  WriteDword(editor->raw, 275, 0xFFFFFFFF);
  WriteDword(editor->raw, 128, 0);
  WriteDword(editor->raw, 132, 0);
  WriteDword(editor->raw, 120, 0);
  editor->raw[163] = 0;
  editor->raw[164] = 0;
  editor->raw[165] = 0;
  editor->raw[166] = 0;
  editor->raw[162] = 0;
  editor->raw[136] = 0xFF;
  editor->raw[137] = 0xFF;
  WriteDword(editor->raw, 158, ::GetTickCount());
  WriteDword(editor->raw, 263,
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(render->CreateLine())));
  WriteDword(editor->raw, 267,
             static_cast<unsigned>(
                 reinterpret_cast<uintptr_t>(render->CreateFontFace(22, 400, "SAMPAUX3"))));
  WriteDword(editor->raw, 271,
             static_cast<unsigned>(
                 reinterpret_cast<uintptr_t>(render->CreateFontFace(28, 400, "SAMPAUX3"))));
  return editor;
}

PickupInfo *CreatePickupInfo() {
  PickupInfo *pickup = new PickupInfo();
  WriteDword(pickup->raw, 0, 0);
  pickup->raw[4] = 0xFF;
  pickup->raw[5] = 0xFF;
  return pickup;
}

SpriteWithDevice *CreateSpriteWithDevice(unsigned device) {
  RenderDevice *render = &NullRenderDevice();
  SpriteWithDevice *sprite = new SpriteWithDevice();
  WriteDword(sprite->raw, 0, device);
  WriteDword(sprite->raw, 4,
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(render->CreateSprite())));
  return sprite;
}

PreviewState *CreatePreviewState() {
  PreviewState *preview = new PreviewState();
  WriteDword(preview->raw, 0, 0);
  WriteDword(preview->raw, 4, 0);
  WriteDword(preview->raw, 12, 0);
  WriteDword(preview->raw, 24, 0);
  WriteDword(preview->raw, 8, 0);
  return preview;
}

}  // namespace samp::ui
