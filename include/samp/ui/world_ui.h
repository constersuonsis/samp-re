#pragma once

namespace samp::ui {

struct TextLabelPool {
  unsigned char raw[0x44A20] = {};
};

struct ObjectEditor {
  unsigned char raw[0x117] = {};
};

struct PickupInfo {
  unsigned char raw[6] = {};
};

struct SpriteWithDevice {
  unsigned char raw[8] = {};
};

struct PreviewState {
  unsigned char raw[0x1C] = {};
};

TextLabelPool *CreateTextLabelPool();
ObjectEditor *CreateObjectEditor(unsigned device);
PickupInfo *CreatePickupInfo();
SpriteWithDevice *CreateSpriteWithDevice(unsigned device);
PreviewState *CreatePreviewState();

}  // namespace samp::ui
