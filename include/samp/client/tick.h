#pragma once

namespace samp::client {

int ProcessClientTick(bool paused, int init_arg);
void SyncTimeOfDay(unsigned char hour, unsigned char minute);
bool HasClient();
bool ClientOverlayFlag();
void DestroyClient();

}  // namespace samp::client
