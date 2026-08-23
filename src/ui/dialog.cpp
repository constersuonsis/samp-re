#include "samp/ui/dialog.h"

#include <string>

namespace samp::ui {

namespace {

struct ShowDialogMessage {
    protocol::DialogHeader header;
    std::string body;
};

bool ReadShowDialogMessage(samp::net::BitStream& stream, ShowDialogMessage& message) {
    message = ShowDialogMessage{};
    if (!protocol::ReadDialogHeader(stream, message.header)) {
        return false;
    }
    return protocol::ReadCompressedText(stream, message.body);
}

}

void BindDialog(protocol::SampClient& client, DialogState& state) {
    client.Dispatcher().On<ShowDialogMessage>(
        protocol::RpcId::ShowDialog, &ReadShowDialogMessage,
        [&state](const ShowDialogMessage& message) {
            state.open = true;
            state.header = message.header;
            state.body = message.body;
        });
}

bool SubmitDialog(protocol::SampClient& client, DialogState& state,
                  protocol::DialogButton button, std::int16_t listIndex,
                  const std::string& inputText, bool hasInput) {
    if (!state.open) {
        return false;
    }

    protocol::DialogResponse response;
    response.dialogId = state.header.dialogId;
    response.button = button;
    response.listIndex = listIndex;
    response.hasInput = hasInput;
    response.inputText = inputText;

    net::BitStream payload;
    WriteDialogResponse(payload, response);
    if (!client.SendRpc(protocol::RpcId::DialogResponse, payload)) {
        return false;
    }

    state.open = false;
    state.body.clear();
    return true;
}

}
