#include "samp/vehicle/vehicle_pool_sync.h"

#include "samp/protocol/rpc_payloads.h"

namespace samp::vehicle {

void BindVehiclePool(protocol::SampClient& client, VehiclePool& pool) {
    protocol::RpcDispatcher& dispatcher = client.Dispatcher();

    dispatcher.On<protocol::WorldVehicleAdd>(
        protocol::RpcId::WorldVehicleAdd, &protocol::ReadWorldVehicleAdd,
        [&pool](const protocol::WorldVehicleAdd& message) {
            pool.Add(message.vehicleId, message.modelId, message.colour1, message.colour2);
        });

    dispatcher.On<protocol::WorldVehicleRemove>(
        protocol::RpcId::WorldVehicleRemove, &protocol::ReadWorldVehicleRemove,
        [&pool](const protocol::WorldVehicleRemove& message) {
            pool.Remove(message.vehicleId);
        });
}

}
