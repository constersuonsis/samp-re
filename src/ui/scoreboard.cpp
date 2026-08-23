#include "samp/ui/scoreboard.h"

#include <algorithm>

namespace samp::ui {

std::vector<ScoreboardRow> BuildScoreboardView(const samp::player::PlayerPool& pool,
                                               ScoreboardSort sort) {
    std::vector<ScoreboardRow> rows;
    rows.reserve(pool.ConnectedCount());

    for (std::uint16_t id = 0; id <= samp::protocol::kMaxPlayerId; ++id) {
        const samp::player::PlayerRecord* record = pool.Get(id);
        if (record == nullptr) {
            continue;
        }
        rows.push_back({id, record->name, record->score, record->ping});
    }

    const bool byPing = sort == ScoreboardSort::ByPing;
    std::stable_sort(rows.begin(), rows.end(),
                     [byPing](const ScoreboardRow& a, const ScoreboardRow& b) {
                         const int keyA = byPing ? a.ping : a.score;
                         const int keyB = byPing ? b.ping : b.score;
                         if (keyA != keyB) {
                             return keyA > keyB;
                         }
                         return a.playerId < b.playerId;
                     });
    return rows;
}

}
