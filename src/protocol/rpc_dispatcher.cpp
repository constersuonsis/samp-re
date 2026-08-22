#include "samp/protocol/rpc_dispatcher.h"

#include <utility>

namespace samp::protocol {

void RpcDispatcher::On(RpcId id, EmptyHandler handler) {
    handlers_[ToByte(id)] = [handler = std::move(handler)](net::BitStream&) {
        handler();
        return true;
    };
}

DispatchResult RpcDispatcher::Dispatch(RpcId id, net::BitStream& payload) const {
    const auto entry = handlers_.find(ToByte(id));
    if (entry == handlers_.end()) {
        return DispatchResult::Unregistered;
    }

    return entry->second(payload) ? DispatchResult::Handled : DispatchResult::Malformed;
}

bool RpcDispatcher::IsRegistered(RpcId id) const {
    return handlers_.find(ToByte(id)) != handlers_.end();
}

void RpcDispatcher::Clear() {
    handlers_.clear();
}

}  // namespace samp::protocol
