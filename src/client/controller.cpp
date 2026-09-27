#include "samp/client/controller.h"
#include "samp/game/memory.h"

#include <cstddef>
#include <cstring>

namespace samp::client {
namespace {

struct StatusBlock {
  unsigned code = 0;
  unsigned char flag = 0;
};

struct GameLink {
  unsigned context = 0;
  unsigned game_data = 0xB6F39C;
};

unsigned char g_shared_state[0x348] = {};
ClientController *g_main_controller = nullptr;

void WriteField(ClientController &controller, size_t offset, unsigned value) {
  std::memcpy(controller.fields + offset - 8, &value, sizeof(value));
}

}  // namespace

void InitClientController(ClientController &controller) {
  StatusBlock *status = new StatusBlock();
  GameLink *link = new GameLink();
  controller.status = status;
  controller.game_link = link;
  controller.fields[0] = 0;
  WriteField(controller, 41, 0);
  WriteField(controller, 49, 0);
  WriteField(controller, 77, 0);
  WriteField(controller, 85, 0);
  WriteField(controller, 89, 1);
  WriteField(controller, 93, 90);
  WriteField(controller, 97, 0);
  WriteField(controller, 101, 0);
  WriteField(controller, 105, 0);
  controller.fields[109 - 8] = 0;
  std::memset(g_shared_state, 0, sizeof(g_shared_state));
  std::memset(controller.fields + 110 - 8, 0, 0xD4);
}

unsigned ControllerTickRate(const ClientController &controller) {
  unsigned value = 0;
  std::memcpy(&value, controller.fields + 89 - 8, sizeof(value));
  return value;
}

unsigned ControllerFrameLimit(const ClientController &controller) {
  unsigned value = 0;
  std::memcpy(&value, controller.fields + 93 - 8, sizeof(value));
  return value;
}

void SetControllerFrameLimit(ClientController &controller, unsigned limit) {
  WriteField(controller, 93, limit);
}

void SetControllerHeadMovementEnabled(ClientController &controller, bool enabled) {
  WriteField(controller, 89, enabled ? 1U : 0U);
}

void SetControllerGravity(ClientController &controller, bool enabled) {
  unsigned char time_format[12] = {};
  if (enabled) {
    std::memcpy(time_format, "%02d:%02d", sizeof("%02d:%02d"));
    samp::game::WriteByte(reinterpret_cast<void *>(0x52CF10), 0x56);
    WriteField(controller, 105, 1);
  } else {
    samp::game::WriteByte(reinterpret_cast<void *>(0x52CF10), 0xC3);
    WriteField(controller, 105, 0);
  }
  samp::game::WriteBytes(reinterpret_cast<void *>(0x859A6C), time_format,
                         sizeof(time_format));
}

void SetMainController(ClientController *controller) {
  g_main_controller = controller;
}

ClientController *MainController() {
  return g_main_controller;
}

}  // namespace samp::client
