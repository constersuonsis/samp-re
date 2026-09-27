#pragma once

namespace samp::client {

bool HasMainController();
void SetMainControllerPresent(bool present);
unsigned RunInitThread();

}  // namespace samp::client
