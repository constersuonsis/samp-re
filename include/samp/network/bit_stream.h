#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace samp::net {

class BitStream {
public:
    static constexpr std::size_t kStackCapacityBytes = 256;
    static constexpr int kStackCapacityBits = static_cast<int>(kStackCapacityBytes) * 8;

    BitStream();

    explicit BitStream(std::size_t initialBytes);

    BitStream(const void* data, std::size_t sizeInBytes, bool copyData);

    ~BitStream();

    BitStream(const BitStream&) = delete;
    BitStream& operator=(const BitStream&) = delete;

    void WriteBit(bool value);

    void WriteBits(const void* input, int bitCount, bool rightAlignBits = true);

    void WriteBytes(const void* input, std::size_t byteCount);

    void WriteAlignedBytes(const void* input, std::size_t byteCount);

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

    void SetData(unsigned char* data);

    int CopyData(unsigned char** output) const;

    void CopyBitsFrom(BitStream& source, int bitCount);

    static std::size_t BitsToBytes(int bitCount) {
        return static_cast<std::size_t>((bitCount + 7) >> 3);
    }

private:

    void AddBitsAndReallocate(int bitCount);

    bool OwnsHeapBuffer() const { return copyData_ && data_ != stackData_; }

    int bitsUsed_;
    int bitsAllocated_;
    int readOffset_;
    unsigned char* data_;
    bool copyData_;
    unsigned char stackData_[kStackCapacityBytes];
};

}
