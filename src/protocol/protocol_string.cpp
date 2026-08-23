#include "samp/protocol/protocol_string.h"

#include <algorithm>
#include <cstdint>

namespace samp::protocol {
namespace {

bool ReadBody(net::BitStream& stream, std::string& value, std::size_t length) {
    value.clear();
    if (length == 0) {
        return true;
    }

    value.resize(length);
    if (!stream.ReadBytes(value.data(), length)) {
        value.clear();
        return false;
    }
    return true;
}

}

bool ReadString8(net::BitStream& stream, std::string& value) {
    std::uint8_t length = 0;
    if (!stream.Read(length)) {
        value.clear();
        return false;
    }
    return ReadBody(stream, value, length);
}

void WriteString8(net::BitStream& stream, std::string_view value) {
    const std::size_t length = std::min(value.size(), kMaxStringLength);
    stream.Write(static_cast<std::uint8_t>(length));
    stream.WriteBytes(value.data(), length);
}

bool ReadString32(net::BitStream& stream, std::string& value, std::size_t maxLength) {
    std::uint32_t length = 0;
    if (!stream.Read(length)) {
        value.clear();
        return false;
    }

    if (length > maxLength) {
        value.clear();
        return false;
    }

    return ReadBody(stream, value, length);
}

void WriteString32(net::BitStream& stream, std::string_view value) {
    const std::size_t length = std::min(value.size(), kMaxStringLength);
    stream.Write(static_cast<std::uint32_t>(length));
    stream.WriteBytes(value.data(), length);
}

}
