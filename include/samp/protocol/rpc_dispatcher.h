#pragma once

#include "samp/network/bit_stream.h"
#include "samp/protocol/rpc_ids.h"

#include <cstddef>
#include <functional>
#include <unordered_map>

namespace samp::protocol {

/// What became of a packet handed to the dispatcher.
enum class DispatchResult {
    /// A handler ran.
    Handled,
    /// Nothing is bound to this id. Normal: a client need not answer everything.
    Unregistered,
    /// A handler is bound but the payload did not parse. The connection is
    /// either out of step or the packet was tampered with.
    Malformed,
};

/// Routes an incoming message to whatever is bound to its id.
///
/// Binding a message type also binds its reader, so a handler never sees a
/// stream — only a parsed message. Ids that carry no body take a handler with
/// no argument, which keeps the caller from inventing a payload for them.
class RpcDispatcher {
public:
    template <typename Message>
    using MessageHandler = std::function<void(const Message&)>;

    using EmptyHandler = std::function<void()>;

    /// Binds a reader and a handler. Binding the same id twice replaces the
    /// first binding.
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

    /// Binds a handler for an id whose message has no body.
    void On(RpcId id, EmptyHandler handler);

    DispatchResult Dispatch(RpcId id, net::BitStream& payload) const;

    bool IsRegistered(RpcId id) const;

    void Clear();

    std::size_t Size() const { return handlers_.size(); }

private:
    /// Returns false when the payload could not be parsed.
    using Entry = std::function<bool(net::BitStream&)>;

    std::unordered_map<std::uint8_t, Entry> handlers_;
};

}  // namespace samp::protocol
