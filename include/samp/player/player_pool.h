#pragma once

#include "samp/protocol/pool_limits.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace samp::player {

inline constexpr std::size_t kMaxPlayerNameLength = 24;

struct PlayerRecord {
    bool connected = false;
    bool isNpc = false;

    bool hasColour = false;
    std::uint32_t colour = 0;
    std::string name;
    std::int32_t score = 0;
    std::int32_t ping = 0;
};

class PlayerPool {
public:
    static constexpr std::size_t kCapacity =
        static_cast<std::size_t>(protocol::kMaxPlayerId) + 1;

    bool Add(std::uint16_t playerId, std::string name, std::uint32_t colour, bool isNpc);

    bool Remove(std::uint16_t playerId);

    bool UpdateScore(std::uint16_t playerId, std::int32_t score, std::int32_t ping);

    bool SetColour(std::uint16_t playerId, std::uint32_t colour);

    bool Rename(std::uint16_t playerId, std::string name);

    const PlayerRecord* Get(std::uint16_t playerId) const;

    std::size_t ConnectedCount() const { return connectedCount_; }

private:
    PlayerRecord* Editable(std::uint16_t playerId);

    std::array<PlayerRecord, kCapacity> players_{};
    std::size_t connectedCount_ = 0;
};

}
