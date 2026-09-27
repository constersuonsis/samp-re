#pragma once

namespace samp::client {

struct ClientController {
  void *status = nullptr;
  void *game_link = nullptr;
  unsigned char fields[0x142 - 8] = {};
};

void InitClientController(ClientController &controller);
unsigned ControllerTickRate(const ClientController &controller);
unsigned ControllerFrameLimit(const ClientController &controller);
void SetControllerFrameLimit(ClientController &controller, unsigned limit);
void SetControllerHeadMovementEnabled(ClientController &controller, bool enabled);
void SetControllerGravity(ClientController &controller, bool enabled);
void SetMainController(ClientController *controller);
ClientController *MainController();

}  // namespace samp::client
