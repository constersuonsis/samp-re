#include "samp/network/bit_stream.h"

#include <cstdlib>

namespace samp::net {

BitStream::BitStream()
    : bitsUsed_(0),
      bitsAllocated_(kStackCapacityBits),
      readOffset_(0),
      data_(stackData_),
      copyData_(true) {}

BitStream::BitStream(std::size_t initialBytes)
    : bitsUsed_(0), readOffset_(0), copyData_(true) {
    if (initialBytes > kStackCapacityBytes) {
        data_ = static_cast<unsigned char*>(std::malloc(initialBytes));
        bitsAllocated_ = static_cast<int>(initialBytes) * 8;
    } else {
        data_ = stackData_;
        bitsAllocated_ = kStackCapacityBits;
    }
}

BitStream::BitStream(const void* data, std::size_t sizeInBytes, bool copyData)
    : bitsUsed_(static_cast<int>(sizeInBytes) * 8),
      bitsAllocated_(static_cast<int>(sizeInBytes) * 8),
      readOffset_(0),
      data_(nullptr),
      copyData_(copyData) {
    if (!copyData) {

        data_ = static_cast<unsigned char*>(const_cast<void*>(data));
        return;
    }

    if (sizeInBytes == 0) {
        return;
    }

    if (sizeInBytes > kStackCapacityBytes) {
        data_ = static_cast<unsigned char*>(std::malloc(sizeInBytes));
    } else {
        data_ = stackData_;
        bitsAllocated_ = kStackCapacityBits;
    }
    std::memcpy(data_, data, sizeInBytes);
}

BitStream::~BitStream() {
    if (OwnsHeapBuffer()) {
        std::free(data_);
    }
}

void BitStream::AddBitsAndReallocate(int bitCount) {
    if (bitCount <= 0) {
        return;
    }

    int newBitsAllocated = bitsUsed_ + bitCount;
    if (newBitsAllocated > 0 && ((bitsAllocated_ - 1) >> 3) < ((newBitsAllocated - 1) >> 3)) {

        newBitsAllocated = (bitsUsed_ + bitCount) * 2;
        const std::size_t bytesNeeded = BitsToBytes(newBitsAllocated);

        if (data_ == stackData_) {
            if (bytesNeeded > kStackCapacityBytes) {
                unsigned char* heapBuffer = static_cast<unsigned char*>(std::malloc(bytesNeeded));
                std::memcpy(heapBuffer, stackData_, BitsToBytes(bitsAllocated_));
                data_ = heapBuffer;
            }
        } else {
            data_ = static_cast<unsigned char*>(std::realloc(data_, bytesNeeded));
        }
    }

    if (newBitsAllocated > bitsAllocated_) {
        bitsAllocated_ = newBitsAllocated;
    }
}

void BitStream::WriteBit(bool value) {
    AddBitsAndReallocate(1);

    const int bitInByte = bitsUsed_ & 7;
    unsigned char& target = data_[bitsUsed_ >> 3];

    if (bitInByte == 0) {
        target = value ? 0x80 : 0x00;
    } else if (value) {
        target |= static_cast<unsigned char>(0x80 >> bitInByte);
    }

    ++bitsUsed_;
}

void BitStream::WriteBits(const void* input, int bitCount, bool rightAlignBits) {
    if (bitCount <= 0) {
        return;
    }

    AddBitsAndReallocate(bitCount);

    const unsigned char* source = static_cast<const unsigned char*>(input);
    const int bitInByte = bitsUsed_ & 7;
    int remaining = bitCount;

    while (remaining > 0) {
        unsigned char value = *source;
        if (remaining < 8 && rightAlignBits) {
            value = static_cast<unsigned char>(value << (8 - remaining));
        }

        if (bitInByte == 0) {
            data_[bitsUsed_ >> 3] = value;
        } else {
            data_[bitsUsed_ >> 3] |= static_cast<unsigned char>(value >> bitInByte);

            const int spaceLeftInByte = 8 - bitInByte;
            if (spaceLeftInByte < remaining) {
                data_[(bitsUsed_ >> 3) + 1] = static_cast<unsigned char>(value << spaceLeftInByte);
            }
        }

        bitsUsed_ += (remaining >= 8) ? 8 : remaining;
        remaining -= 8;
        ++source;
    }
}

void BitStream::WriteBytes(const void* input, std::size_t byteCount) {
    if (byteCount == 0 || input == nullptr) {
        return;
    }

    if ((bitsUsed_ & 7) != 0) {
        WriteBits(input, static_cast<int>(byteCount) * 8, true);
        return;
    }

    AddBitsAndReallocate(static_cast<int>(byteCount) * 8);
    std::memcpy(data_ + BitsToBytes(bitsUsed_), input, byteCount);
    bitsUsed_ += static_cast<int>(byteCount) * 8;
}

void BitStream::WriteAlignedBytes(const void* input, std::size_t byteCount) {
    AlignWriteToByteBoundary();
    WriteBytes(input, byteCount);
}

void BitStream::WriteCompressedBits(const void* input, int bitCount, bool unsignedData) {
    const unsigned char* source = static_cast<const unsigned char*>(input);
    const unsigned char redundantByte = unsignedData ? 0x00 : 0xFF;

    int currentByte = (bitCount >> 3) - 1;

    while (currentByte > 0) {
        if (source[currentByte] != redundantByte) {
            WriteBit(false);
            WriteBits(source, (currentByte + 1) * 8, true);
            return;
        }
        WriteBit(true);
        --currentByte;
    }

    const unsigned char lastByte = source[currentByte];
    const bool upperNibbleRedundant =
        unsignedData ? (lastByte & 0xF0) == 0x00 : (lastByte & 0xF0) == 0xF0;

    WriteBit(upperNibbleRedundant);
    WriteBits(source + currentByte, upperNibbleRedundant ? 4 : 8, true);
}

bool BitStream::ReadBit() {
    const unsigned char mask = static_cast<unsigned char>(0x80 >> (readOffset_ & 7));
    const bool value = (data_[readOffset_ >> 3] & mask) != 0;
    ++readOffset_;
    return value;
}

bool BitStream::ReadBit(bool& value) {
    if (readOffset_ + 1 > bitsUsed_) {
        return false;
    }
    value = ReadBit();
    return true;
}

bool BitStream::ReadBits(void* output, int bitCount, bool rightAlignBits) {
    if (bitCount <= 0 || readOffset_ + bitCount > bitsUsed_) {
        return false;
    }

    unsigned char* destination = static_cast<unsigned char*>(output);
    std::memset(destination, 0, BitsToBytes(bitCount));

    const int bitInByte = readOffset_ & 7;
    int remaining = bitCount;

    while (remaining > 0) {
        *destination |= static_cast<unsigned char>(data_[readOffset_ >> 3] << bitInByte);

        if (bitInByte > 0 && remaining > 8 - bitInByte) {
            *destination |= static_cast<unsigned char>(data_[(readOffset_ >> 3) + 1] >> (8 - bitInByte));
        }

        remaining -= 8;
        if (remaining < 0) {
            if (rightAlignBits) {
                *destination >>= -remaining;
            }
            readOffset_ += 8 + remaining;
        } else {
            readOffset_ += 8;
        }
        ++destination;
    }

    return true;
}

bool BitStream::ReadBytes(void* output, std::size_t byteCount) {
    const int bitCount = static_cast<int>(byteCount) * 8;

    if ((readOffset_ & 7) != 0) {
        return ReadBits(output, bitCount, true);
    }

    if (readOffset_ + bitCount > bitsUsed_) {
        return false;
    }

    std::memcpy(output, data_ + (readOffset_ >> 3), byteCount);
    readOffset_ += bitCount;
    return true;
}

bool BitStream::ReadAlignedBytes(void* output, std::size_t byteCount) {
    if (byteCount == 0) {
        return false;
    }

    AlignReadToByteBoundary();
    if (readOffset_ + static_cast<int>(byteCount) * 8 > bitsUsed_) {
        return false;
    }

    std::memcpy(output, data_ + (readOffset_ >> 3), byteCount);
    readOffset_ += static_cast<int>(byteCount) * 8;
    return true;
}

bool BitStream::ReadCompressedBits(void* output, int bitCount, bool unsignedData) {
    unsigned char* destination = static_cast<unsigned char*>(output);
    const unsigned char redundantByte = unsignedData ? 0x00 : 0xFF;
    const unsigned char redundantNibble = unsignedData ? 0x00 : 0xF0;

    int currentByte = (bitCount >> 3) - 1;

    while (currentByte > 0) {
        bool isRedundant = false;
        if (!ReadBit(isRedundant)) {
            return false;
        }

        if (!isRedundant) {
            return ReadBits(destination, (currentByte + 1) * 8, true);
        }

        destination[currentByte] = redundantByte;
        --currentByte;
    }

    bool upperNibbleRedundant = false;
    if (!ReadBit(upperNibbleRedundant)) {
        return false;
    }

    if (!upperNibbleRedundant) {
        return ReadBits(destination + currentByte, 8, true);
    }

    if (!ReadBits(destination + currentByte, 4, true)) {
        return false;
    }
    destination[currentByte] |= redundantNibble;
    return true;
}

void BitStream::Reset() {
    bitsUsed_ = 0;
    readOffset_ = 0;
}

void BitStream::ResetReadPointer() {
    readOffset_ = 0;
}

void BitStream::AlignWriteToByteBoundary() {
    if (bitsUsed_ != 0) {
        bitsUsed_ += 8 - (((bitsUsed_ - 1) & 7) + 1);
    }
}

void BitStream::AlignReadToByteBoundary() {
    if (readOffset_ != 0) {
        readOffset_ += 8 - (((readOffset_ - 1) & 7) + 1);
    }
}

void BitStream::IgnoreBits(int bitCount) {
    readOffset_ += bitCount;
}

void BitStream::IgnoreBytes(std::size_t byteCount) {
    readOffset_ += static_cast<int>(byteCount) * 8;
}

void BitStream::SetData(unsigned char* data) {
    if (OwnsHeapBuffer()) {
        std::free(data_);
    }
    data_ = data;
    copyData_ = false;
}

int BitStream::CopyData(unsigned char** output) const {
    const std::size_t byteCount = BitsToBytes(bitsUsed_);
    *output = new unsigned char[byteCount];
    std::memcpy(*output, data_, byteCount);
    return bitsUsed_;
}

void BitStream::CopyBitsFrom(BitStream& source, int bitCount) {
    AddBitsAndReallocate(bitCount);

    for (int i = 0; i < bitCount; ++i) {
        bool bit = false;
        if (!source.ReadBit(bit)) {
            return;
        }
        WriteBit(bit);
    }
}

}
