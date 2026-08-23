#pragma once

#include "samp/protocol/pool_limits.h"
#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/samp_client.h"

#include <array>
#include <cstddef>
#include <string>

namespace samp::ui {

struct TextDrawRecord {
    bool exists = false;
    protocol::TextDrawStyle style;
    std::string text;
};

class TextDrawManager {
public:
    static constexpr std::size_t kCapacity =
        static_cast<std::size_t>(protocol::kTextDrawPoolSize);

    bool Show(std::uint16_t textDrawId, const protocol::TextDrawStyle& style,
              std::string text);

    bool SetString(std::uint16_t textDrawId, std::string text);

    bool Hide(std::uint16_t textDrawId);

    const TextDrawRecord* Get(std::uint16_t textDrawId) const;

private:
    std::array<TextDrawRecord, kCapacity> draws_{};
};

void BindTextDraws(protocol::SampClient& client, TextDrawManager& manager);

}
