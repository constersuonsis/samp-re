#include "samp/ui/text_draw.h"

namespace samp::ui {

const TextDrawRecord* TextDrawManager::Get(std::uint16_t textDrawId) const {
    if (!protocol::IsValidTextDrawId(textDrawId)) {
        return nullptr;
    }
    const TextDrawRecord& record = draws_[textDrawId];
    return record.exists ? &record : nullptr;
}

bool TextDrawManager::Show(std::uint16_t textDrawId, const protocol::TextDrawStyle& style,
                           std::string text) {
    if (!protocol::IsValidTextDrawId(textDrawId)) {
        return false;
    }

    TextDrawRecord& record = draws_[textDrawId];
    record.exists = true;
    record.style = style;
    record.text = std::move(text);
    return true;
}

bool TextDrawManager::SetString(std::uint16_t textDrawId, std::string text) {
    if (!protocol::IsValidTextDrawId(textDrawId)) {
        return false;
    }

    TextDrawRecord& record = draws_[textDrawId];
    if (!record.exists) {

        return false;
    }

    record.text = std::move(text);
    return true;
}

bool TextDrawManager::Hide(std::uint16_t textDrawId) {
    if (!protocol::IsValidTextDrawId(textDrawId)) {
        return false;
    }

    TextDrawRecord& record = draws_[textDrawId];
    if (!record.exists) {
        return false;
    }

    record = TextDrawRecord{};
    return true;
}

void BindTextDraws(protocol::SampClient& client, TextDrawManager& manager) {
    protocol::RpcDispatcher& dispatcher = client.Dispatcher();

    dispatcher.On<protocol::ShowTextDraw>(
        protocol::RpcId::ShowTextDraw, &protocol::ReadShowTextDraw,
        [&manager](const protocol::ShowTextDraw& message) {
            manager.Show(message.textDrawId, message.style, message.text);
        });

    dispatcher.On<protocol::SetTextDrawString>(
        protocol::RpcId::SetTextDrawString, &protocol::ReadSetTextDrawString,
        [&manager](const protocol::SetTextDrawString& message) {
            manager.SetString(message.textDrawId, message.text);
        });

    dispatcher.On<protocol::HideTextDraw>(
        protocol::RpcId::HideTextDraw, &protocol::ReadHideTextDraw,
        [&manager](const protocol::HideTextDraw& message) {
            manager.Hide(message.textDrawId);
        });
}

}
