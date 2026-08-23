#include "samp/player/player_pool_sync.h"

#include "samp/protocol/rpc_payloads.h"

namespace samp::player {

void BindPlayerPool(protocol::SampClient& client, PlayerPool& pool) {
    protocol::RpcDispatcher& dispatcher = client.Dispatcher();

    dispatcher.On<protocol::ServerJoin>(
        protocol::RpcId::ServerJoin, &protocol::ReadServerJoin,
        [&pool](const protocol::ServerJoin& message) {
            pool.Add(message.playerId, message.name,
                     static_cast<std::uint32_t>(message.colour), message.isNpc != 0);
        });

    dispatcher.On<protocol::ServerQuit>(
        protocol::RpcId::ServerQuit, &protocol::ReadServerQuit,
        [&pool](const protocol::ServerQuit& message) { pool.Remove(message.playerId); });

    dispatcher.On<protocol::WorldPlayerRemove>(
        protocol::RpcId::WorldPlayerRemove, &protocol::ReadWorldPlayerRemove,
        [&pool](const protocol::WorldPlayerRemove& message) {
            pool.Remove(message.playerId);
        });

    dispatcher.On<protocol::SetPlayerColour>(
        protocol::RpcId::SetPlayerColour, &protocol::ReadSetPlayerColour,
        [&pool](const protocol::SetPlayerColour& message) {
            pool.SetColour(message.playerId, static_cast<std::uint32_t>(message.colour));
        });

    dispatcher.On<protocol::SetPlayerName>(
        protocol::RpcId::SetPlayerName, &protocol::ReadSetPlayerName,
        [&pool](const protocol::SetPlayerName& message) {
            if (message.Accepted()) {
                pool.Rename(message.playerId, message.name);
            }
        });

    dispatcher.On<std::vector<protocol::ScoreEntry>>(
        protocol::RpcId::UpdateScoresPingsIPs, &protocol::ReadScoreUpdate,
        [&pool](const std::vector<protocol::ScoreEntry>& entries) {
            for (const protocol::ScoreEntry& entry : entries) {
                pool.UpdateScore(entry.playerId, entry.score, entry.ping);
            }
        });
}

}
