#include "samp/ui/dialog.h"
#include "samp/ui/render_device.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

static_assert(sizeof(DialogSystem) == 0x24);

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

}  // namespace

DialogSystem *CreateDialogSystem(void *device) {
  if (!device) {
    return nullptr;
  }
  DialogSystem *system = new DialogSystem();
  system->device = device;
  system->sprite = Direct3DRenderDevice(device).CreateSprite();
  if (system->sprite) {
    using ResetSpriteFunction = unsigned long(__stdcall *)(void *);
    void **sprite_vtable = *reinterpret_cast<void ***>(system->sprite);
    reinterpret_cast<ResetSpriteFunction>(sprite_vtable[13])(system->sprite);
  }
  using CreateStateBlockFunction = HRESULT(__stdcall *)(void *, int, void **);
  void **device_vtable = *reinterpret_cast<void ***>(device);
  reinterpret_cast<CreateStateBlockFunction>(device_vtable[59])(device, 1,
                                                                &system->state_block);
  return system;
}

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
