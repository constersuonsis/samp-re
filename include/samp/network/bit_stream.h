#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace samp::net {

/// Bit-level serializer behind every packet and RPC in the client protocol.
///
/// Streams start out using an internal buffer and only move to the heap once
/// they outgrow it, which keeps the common case (small sync packets built and
/// discarded every frame) free of allocations.
class BitStream {
public:
    static constexpr std::size_t kStackCapacityBytes = 256;
    static constexpr int kStackCapacityBits = static_cast<int>(kStackCapacityBytes) * 8;

    BitStream();

    /// Pre-sizes the stream for `initialBytes` of payload.
    explicit BitStream(std::size_t initialBytes);

    /// Wraps an existing buffer. With `copyData` set the bytes are copied into
    /// the stream; otherwise the stream borrows them and the caller must keep
    /// the buffer alive for as long as the stream is used.
    BitStream(const void* data, std::size_t sizeInBytes, bool copyData);

    ~BitStream();

    BitStream(const BitStream&) = delete;
    BitStream& operator=(const BitStream&) = delete;

    // --- Writing -----------------------------------------------------------

    void WriteBit(bool value);

    /// Writes `bitCount` bits from `input`. When the count is not a multiple of
    /// eight, `rightAlignBits` selects whether the trailing partial byte is
    /// taken from its low bits (true) or its high bits (false).
    void WriteBits(const void* input, int bitCount, bool rightAlignBits = true);

    /// Appends raw bytes at the current bit position, without padding. Pairs
    /// with ReadBytes().
    void WriteBytes(const void* input, std::size_t byteCount);

    /// Pads to the next byte boundary first, so the payload can be memcpy'd
    /// straight out again. Pairs with ReadAlignedBytes().
    void WriteAlignedBytes(const void* input, std::size_t byteCount);

    /// Drops leading bytes that carry no information: 0x00 runs for unsigned
    /// values, 0xFF runs for negative ones. The final byte is sent as a nibble
    /// when its upper half is redundant too.
    void WriteCompressedBits(const void* input, int bitCount, bool unsignedData);

    template <typename T>
    void Write(const T& value) {
        static_assert(std::is_arithmetic_v<T>, "BitStream::Write expects an arithmetic type");
        WriteBits(&value, sizeof(T) * 8, true);
    }

    template <typename T>
    void WriteCompressed(const T& value) {
        static_assert(std::is_integral_v<T>, "BitStream::WriteCompressed expects an integral type");
        WriteCompressedBits(&value, sizeof(T) * 8, std::is_unsigned_v<T>);
    }

    // --- Reading -----------------------------------------------------------

    /// Reads one bit without bounds checking. Use the `bool&` overload when the
    /// data comes off the wire.
    bool ReadBit();
    bool ReadBit(bool& value);

    bool ReadBits(void* output, int bitCount, bool rightAlignBits = true);
    bool ReadBytes(void* output, std::size_t byteCount);
    bool ReadAlignedBytes(void* output, std::size_t byteCount);
    bool ReadCompressedBits(void* output, int bitCount, bool unsignedData);

    template <typename T>
    bool Read(T& value) {
        static_assert(std::is_arithmetic_v<T>, "BitStream::Read expects an arithmetic type");
        return ReadBits(&value, sizeof(T) * 8, true);
    }

    template <typename T>
    bool ReadCompressed(T& value) {
        static_assert(std::is_integral_v<T>, "BitStream::ReadCompressed expects an integral type");
        return ReadCompressedBits(&value, sizeof(T) * 8, std::is_unsigned_v<T>);
    }

    // --- Position and state ------------------------------------------------

    /// Empties the stream so it can be reused for another packet.
    void Reset();
    void ResetReadPointer();

    void AlignWriteToByteBoundary();
    void AlignReadToByteBoundary();

    void IgnoreBits(int bitCount);
    void IgnoreBytes(std::size_t byteCount);

    int GetNumberOfBitsUsed() const { return bitsUsed_; }
    std::size_t GetNumberOfBytesUsed() const { return BitsToBytes(bitsUsed_); }
    int GetNumberOfUnreadBits() const { return bitsUsed_ - readOffset_; }

    int GetReadOffset() const { return readOffset_; }
    void SetReadOffset(int offsetInBits) { readOffset_ = offsetInBits; }
    void SetWriteOffset(int offsetInBits) { bitsUsed_ = offsetInBits; }

    unsigned char* GetData() { return data_; }
    const unsigned char* GetData() const { return data_; }

    /// Points the stream at an externally owned buffer. Ownership stays with
    /// the caller; the stream will not free it.
    void SetData(unsigned char* data);

    /// Copies the used portion into a newly allocated buffer and returns its
    /// size in bits. The caller takes ownership via `delete[]`.
    int CopyData(unsigned char** output) const;

    /// Appends `bitCount` bits taken from `source`, advancing its read pointer.
    void CopyBitsFrom(BitStream& source, int bitCount);

    static std::size_t BitsToBytes(int bitCount) {
        return static_cast<std::size_t>((bitCount + 7) >> 3);
    }

private:
    /// Grows the buffer so that `bitCount` more bits fit.
    void AddBitsAndReallocate(int bitCount);

    bool OwnsHeapBuffer() const { return copyData_ && data_ != stackData_; }

    int bitsUsed_;
    int bitsAllocated_;
    int readOffset_;
    unsigned char* data_;
    bool copyData_;
    unsigned char stackData_[kStackCapacityBytes];
};

}  // namespace samp::net
