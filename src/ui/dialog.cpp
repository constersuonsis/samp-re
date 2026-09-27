#include "samp/ui/dialog.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

}  // namespace

DialogState *CreateDialogState(unsigned device) {
  DialogState *dialog = new DialogState();
  WriteDword(dialog->raw, 0, device);
  WriteDword(dialog->raw, 28, 0);
  WriteDword(dialog->raw, 40, 0);
  WriteDword(dialog->raw, 52, 0);
  WriteDword(dialog->raw, 48, 0);
  WriteDword(dialog->raw, 44, 0);
  WriteDword(dialog->raw, 12, 600);
  WriteDword(dialog->raw, 16, 300);
  WriteDword(dialog->raw, 20, 100);
  WriteDword(dialog->raw, 24, 30);
  std::memset(dialog->raw + 137, 0, 0x204);
  return dialog;
}

VehicleSelect *CreateVehicleSelect(unsigned device) {
  VehicleSelect *select = new VehicleSelect();
  WriteDword(select->raw, 0, device);
  WriteDword(select->raw, 32, 0);
  WriteDword(select->raw, 36, 0);
  WriteDword(select->raw, 12, 280);
  WriteDword(select->raw, 16, 150);
  WriteDword(select->raw, 20, 210);
  WriteDword(select->raw, 24, 30);
  WriteDword(select->raw, 28, 38);
  return select;
}

VehicleSelectSmall *CreateVehicleSelectSmall(unsigned device) {
  VehicleSelectSmall *select = new VehicleSelectSmall();
  WriteDword(select->raw, 0, 0);
  WriteDword(select->raw, 4, 0);
  WriteDword(select->raw, 8, ::GetTickCount());
  WriteDword(select->raw, 16, 0);
  WriteDword(select->raw, 12, 0);
  WriteDword(select->raw, 20, device);
  return select;
}

}  // namespace samp::ui
