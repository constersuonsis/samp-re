#pragma once

namespace samp::ui {

struct DialogSystem {
  void *state_block = nullptr;
  void *sprite = nullptr;
  unsigned char reserved[24] = {};
  void *device = nullptr;
};

struct DialogState {
  unsigned char raw[0x29D] = {};
};

struct VehicleSelect {
  unsigned char raw[0x28] = {};
};

struct VehicleSelectSmall {
  unsigned char raw[0x18] = {};
};

DialogSystem *CreateDialogSystem(void *device);
DialogState *CreateDialogState(unsigned device);
VehicleSelect *CreateVehicleSelect(unsigned device);
VehicleSelectSmall *CreateVehicleSelectSmall(unsigned device);

}  // namespace samp::ui
