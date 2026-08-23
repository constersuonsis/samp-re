#include "samp/player/player_pool.h"

namespace samp::player {

PlayerRecord* PlayerPool::Editable(std::uint16_t playerId) {
    if (!protocol::IsValidPlayerId(playerId)) {
        return nullptr;
    }
    PlayerRecord& record = players_[playerId];
    return record.connected ? &record : nullptr;
}

bool PlayerPool::Add(std::uint16_t playerId, std::string name, std::uint32_t colour,
                     bool isNpc) {
    if (!protocol::IsValidPlayerId(playerId) || name.size() > kMaxPlayerNameLength) {
        return false;
    }

    PlayerRecord& record = players_[playerId];
    if (!record.connected) {
        ++connectedCount_;
    }

    record.connected = true;
    record.isNpc = isNpc;
    record.hasColour = colour != 0;
    record.colour = colour;
    record.name = std::move(name);
    record.score = 0;
    record.ping = 0;
    return true;
}

bool PlayerPool::Remove(std::uint16_t playerId) {
    PlayerRecord* record = Editable(playerId);
    if (record == nullptr) {
        return false;
    }

    *record = PlayerRecord{};
    --connectedCount_;
    return true;
}

bool PlayerPool::UpdateScore(std::uint16_t playerId, std::int32_t score, std::int32_t ping) {
    PlayerRecord* record = Editable(playerId);
    if (record == nullptr) {
        return false;
    }

    record->score = score;
    record->ping = ping;
    return true;
}

bool PlayerPool::SetColour(std::uint16_t playerId, std::uint32_t colour) {
    PlayerRecord* record = Editable(playerId);
    if (record == nullptr) {
        return false;
    }

    record->hasColour = true;
    record->colour = colour;
    return true;
}

bool PlayerPool::Rename(std::uint16_t playerId, std::string name) {
    if (name.size() > kMaxPlayerNameLength) {
        return false;
    }

    PlayerRecord* record = Editable(playerId);
    if (record == nullptr) {
        return false;
    }

    record->name = std::move(name);
    return true;
}

const PlayerRecord* PlayerPool::Get(std::uint16_t playerId) const {
    if (!protocol::IsValidPlayerId(playerId)) {
        return nullptr;
    }
    const PlayerRecord& record = players_[playerId];
    return record.connected ? &record : nullptr;
}

}
