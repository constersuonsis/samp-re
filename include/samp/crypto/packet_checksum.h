#pragma once

#include <cstddef>
#include <cstdint>

namespace samp::crypto {

class PacketChecksum {
public:
    static constexpr std::uint16_t kInitialKey = 55665;
    static constexpr std::uint16_t kMultiplier = 52845;
    static constexpr std::uint16_t kAddend = 22719;

    void Update(const void* data, std::size_t size);

    std::uint8_t Value() const;

    void Reset();

private:
    std::uint8_t UpdateByte(std::uint8_t value);

    std::uint16_t key_ = kInitialKey;
    std::uint32_t total_ = 0;
};

}
