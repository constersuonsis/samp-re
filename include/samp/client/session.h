#pragma once

namespace samp::client {

bool InitializeSessionOnce();
bool InitializeSessionOnce(int sprite_arg);
bool SessionReady();
bool ConnectSessionClient();
void ProcessSessionTick();
void *SessionDevice();

}  // namespace samp::client
