#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace samp::browser {

struct ServerInfo {
    bool passworded = false;
    std::uint16_t players = 0;
    std::uint16_t maxPlayers = 0;
    std::string hostname;
    std::string gameMode;
    std::string language;
};

std::optional<ServerInfo> QueryInfo(const char* host, std::uint16_t serverPort,
                                    unsigned timeoutMs);

}
