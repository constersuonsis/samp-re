#pragma once

namespace samp::ui {

struct DialogState {
  unsigned char raw[0x29D] = {};
};

struct VehicleSelect {
  unsigned char raw[0x28] = {};
};

struct VehicleSelectSmall {
  unsigned char raw[0x18] = {};
};

DialogState *CreateDialogState(unsigned device);
VehicleSelect *CreateVehicleSelect(unsigned device);
VehicleSelectSmall *CreateVehicleSelectSmall(unsigned device);

}  // namespace samp::ui
