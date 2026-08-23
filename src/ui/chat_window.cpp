#include "samp/ui/chat_window.h"

namespace samp::ui {

void ChatWindow::AddServerLine(std::uint32_t colour, std::string text) {
    ChatLine line;
    line.colour = colour;
    line.text = std::move(text);
    lines_.push_back(std::move(line));
    TrimOverflow();
}

void ChatWindow::AddPlayerLine(std::uint16_t playerId, std::string text) {
    ChatLine line;
    line.playerId = playerId;
    line.text = std::move(text);
    lines_.push_back(std::move(line));
    TrimOverflow();
}

void ChatWindow::TrimOverflow() {
    while (lines_.size() > kMaxChatHistoryLines) {
        lines_.pop_front();
    }
}

void ChatWindow::Clear() {
    lines_.clear();
}

void BindChatWindow(protocol::SampClient& client, ChatWindow& window) {
    protocol::RpcDispatcher& dispatcher = client.Dispatcher();

    dispatcher.On<protocol::ClientMessage>(
        protocol::RpcId::ClientMessage, &protocol::ReadClientMessage,
        [&window](const protocol::ClientMessage& message) {
            window.AddServerLine(message.colour, message.text);
        });

    dispatcher.On<protocol::ChatMessage>(
        protocol::RpcId::Chat, &protocol::ReadChatMessage,
        [&window](const protocol::ChatMessage& message) {
            window.AddPlayerLine(message.playerId, message.text);
        });
}

}
