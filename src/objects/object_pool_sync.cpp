#include "samp/object/object_pool_sync.h"

#include "samp/protocol/rpc_payloads.h"

namespace samp::object {

void BindObjectPool(protocol::SampClient& client, ObjectPool& pool) {
    protocol::RpcDispatcher& dispatcher = client.Dispatcher();

    dispatcher.On<protocol::CreateObject>(
        protocol::RpcId::CreateObject, &protocol::ReadCreateObject,
        [&pool](const protocol::CreateObject& message) {
            pool.Add(message.objectId, message);
        });

    dispatcher.On<protocol::DestroyObject>(
        protocol::RpcId::DestroyObject, &protocol::ReadDestroyObject,
        [&pool](const protocol::DestroyObject& message) {
            pool.Remove(message.objectId);
        });
}

}
