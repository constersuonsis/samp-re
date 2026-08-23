#pragma once

#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/samp_client.h"

#include <string>

namespace samp::ui {

struct DialogState {
    bool open = false;
    protocol::DialogHeader header;
    std::string body;
};

void BindDialog(protocol::SampClient& client, DialogState& state);

bool SubmitDialog(protocol::SampClient& client, DialogState& state,
                  protocol::DialogButton button, std::int16_t listIndex,
                  const std::string& inputText, bool hasInput);

}
