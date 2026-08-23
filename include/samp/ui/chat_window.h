#pragma once

#include "samp/player/player_pool.h"
#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/samp_client.h"

#include <deque>

namespace samp::ui {

struct ChatLine {

    std::uint16_t playerId = protocol::kInvalidPlayerId;
    std::uint32_t colour = 0xFFFFFFFF;
    std::string text;

    bool FromPlayer() const { return playerId != protocol::kInvalidPlayerId; }
};

inline constexpr std::size_t kMaxChatHistoryLines = 128;

class ChatWindow {
public:
    using const_iterator = std::deque<ChatLine>::const_iterator;

    void AddServerLine(std::uint32_t colour, std::string text);

    void AddPlayerLine(std::uint16_t playerId, std::string text);

    void Clear();

    std::size_t Size() const { return lines_.size(); }
    const_iterator begin() const { return lines_.begin(); }
    const_iterator end() const { return lines_.end(); }

private:
    void TrimOverflow();

    std::deque<ChatLine> lines_;
};

void BindChatWindow(protocol::SampClient& client, ChatWindow& window);

}
