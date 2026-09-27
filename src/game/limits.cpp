#include "samp/game/limits.h"
#include "samp/game/memory.h"

namespace samp::game {

void PatchLimits() {
  WaitForByte(reinterpret_cast<volatile unsigned char *>(0x551024), 104);
  const unsigned char header[7] = {0x6A, 0x00, 0x68, 0xC6, 0x02, 0x00, 0x00};
  WriteBytes(reinterpret_cast<void *>(0x551024), header, sizeof(header));
  WriteDword(reinterpret_cast<void *>(0x55105F), 20000);
  WriteDword(reinterpret_cast<void *>(0x5510CF), 4000);
  WriteDword(reinterpret_cast<void *>(0x550F46), 100000);
  WriteDword(reinterpret_cast<void *>(0x550F82), 8000);
  WriteDword(reinterpret_cast<void *>(0x550FBA), 5000);
  WriteDword(reinterpret_cast<void *>(0x551097), 3000);
  WriteByte(reinterpret_cast<void *>(0x550FF2), 0xF0);
  WriteByte(reinterpret_cast<void *>(0x551283), 0xF0);
  WriteByte(reinterpret_cast<void *>(0x551140), 5);
  WriteByte(reinterpret_cast<void *>(0x551178), 1);
  WriteDword(reinterpret_cast<void *>(0x54F3A1), 6000);
  const unsigned char tail[5] = {0x68, 0xFF, 0x7E, 0x00, 0x00};
  WriteBytes(reinterpret_cast<void *>(0x551106), tail, sizeof(tail));
}

}  // namespace samp::game
