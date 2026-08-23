#pragma once

#include "samp/player/player_pool.h"
#include "samp/protocol/samp_client.h"

namespace samp::player {

void BindPlayerPool(protocol::SampClient& client, PlayerPool& pool);

}
