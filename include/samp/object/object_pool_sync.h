#pragma once

#include "samp/object/object_pool.h"
#include "samp/protocol/samp_client.h"

namespace samp::object {

void BindObjectPool(protocol::SampClient& client, ObjectPool& pool);

}
