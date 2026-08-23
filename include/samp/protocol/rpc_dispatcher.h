#pragma once

#include "samp/network/bit_stream.h"
#include "samp/protocol/rpc_ids.h"

#include <cstddef>
#include <functional>
#include <unordered_map>

namespace samp::protocol {

enum class DispatchResult {

    Handled,

    Unregistered,

    Malformed,
};

class RpcDispatcher {
public:
    template <typename Message>
    using MessageHandler = std::function<void(const Message&)>;

    using EmptyHandler = std::function<void()>;

    template <typename Message>
    void On(RpcId id, bool (*read)(net::BitStream&, Message&), MessageHandler<Message> handler) {
        handlers_[ToByte(id)] = [read, handler = std::move(handler)](net::BitStream& payload) {
            Message message;
            if (!read(payload, message)) {
                return false;
            }
            handler(message);
            return true;
        };
    }

    void On(RpcId id, EmptyHandler handler);

    DispatchResult Dispatch(RpcId id, net::BitStream& payload) const;

    bool IsRegistered(RpcId id) const;

    void Clear();

    std::size_t Size() const { return handlers_.size(); }

private:

    using Entry = std::function<bool(net::BitStream&)>;

    std::unordered_map<std::uint8_t, Entry> handlers_;
};

}
