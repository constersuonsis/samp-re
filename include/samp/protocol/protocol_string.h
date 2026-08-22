#pragma once

#include "samp/network/bit_stream.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace samp::protocol {

/// Longest string the protocol ever carries in a length-prefixed field.
inline constexpr std::size_t kMaxStringLength = 255;

/// Strings arrive length-prefixed and without a terminator. Most fields spend a
/// single byte on the length; a few spend four, but the client still refuses
/// anything longer than a byte could have described.

bool ReadString8(net::BitStream& stream, std::string& value);
void WriteString8(net::BitStream& stream, std::string_view value);

bool ReadString32(net::BitStream& stream, std::string& value,
                  std::size_t maxLength = kMaxStringLength);
void WriteString32(net::BitStream& stream, std::string_view value);

}  // namespace samp::protocol
