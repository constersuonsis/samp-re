#pragma once

#include "samp/player/player_pool.h"

#include <vector>

namespace samp::ui {

enum class ScoreboardSort {
    ByScore,
    ByPing,
};

struct ScoreboardRow {
    std::uint16_t playerId = 0;
    std::string name;
    std::int32_t score = 0;
    std::int32_t ping = 0;
};

std::vector<ScoreboardRow> BuildScoreboardView(const samp::player::PlayerPool& pool,
                                               ScoreboardSort sort);

}
