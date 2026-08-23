#include "samp/util/command_processor.h"

#include <cctype>

namespace samp::util {

std::optional<ParsedCommand> ParseCommand(std::string_view chatInput) {
    if (chatInput.empty() || chatInput.front() != '/') {
        return std::nullopt;
    }
    chatInput.remove_prefix(1);

    ParsedCommand parsed;
    const auto space = chatInput.find(' ');
    if (space == std::string_view::npos) {
        parsed.name.assign(chatInput);
    } else {
        parsed.name.assign(chatInput.substr(0, space));

        parsed.arguments.assign(chatInput.substr(space + 1));
    }

    if (parsed.name.empty()) {
        return std::nullopt;
    }
    for (char& c : parsed.name) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return parsed;
}

void CommandProcessor::Register(std::string name, Handler handler) {
    for (char& c : name) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    handlers_[std::move(name)] = std::move(handler);
}

bool CommandProcessor::Dispatch(const ParsedCommand& command) const {
    const auto entry = handlers_.find(command.name);
    if (entry == handlers_.end()) {
        return false;
    }
    entry->second(command.arguments);
    return true;
}

bool CommandProcessor::IsRegistered(std::string_view name) const {
    return handlers_.find(std::string(name)) != handlers_.end();
}

}
