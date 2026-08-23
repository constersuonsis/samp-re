#pragma once

#include "samp/protocol/samp_client.h"
#include "samp/vehicle/vehicle_pool.h"

namespace samp::vehicle {

void BindVehiclePool(protocol::SampClient& client, VehiclePool& pool);

}
