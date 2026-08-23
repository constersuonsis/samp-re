#pragma once

#include "samp/network/bit_stream.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace samp::protocol {

inline constexpr std::size_t kMaxStringLength = 255;

bool ReadString8(net::BitStream& stream, std::string& value);
void WriteString8(net::BitStream& stream, std::string_view value);

bool ReadString32(net::BitStream& stream, std::string& value,
                  std::size_t maxLength = kMaxStringLength);
void WriteString32(net::BitStream& stream, std::string_view value);

}
