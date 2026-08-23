#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace samp::util {

struct ParsedCommand {

    std::string name;

    std::string arguments;
};

std::optional<ParsedCommand> ParseCommand(std::string_view chatInput);

class CommandProcessor {
public:
    using Handler = std::function<void(const std::string& arguments)>;

    void Register(std::string name, Handler handler);

    bool Dispatch(const ParsedCommand& command) const;

    bool IsRegistered(std::string_view name) const;

private:
    std::unordered_map<std::string, Handler> handlers_;
};

}
