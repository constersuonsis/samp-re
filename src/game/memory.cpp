#include "samp/game/memory.h"

#include <windows.h>

namespace samp::game {

bool MakeWritable(void *address, SIZE_T size) {
  DWORD old = 0;
  return ::VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old) != 0;
}

void WriteByte(void *address, unsigned char value) {
  MakeWritable(address, 1);
  *static_cast<volatile unsigned char *>(address) = value;
}

void WriteWord(void *address, unsigned short value) {
  MakeWritable(address, 2);
  *static_cast<volatile unsigned short *>(address) = value;
}

void WriteDword(void *address, unsigned int value) {
  MakeWritable(address, 4);
  *static_cast<volatile unsigned int *>(address) = value;
}

void FillNops(void *address, SIZE_T count) {
  if (!MakeWritable(address, count)) {
    return;
  }
  unsigned char *cell = static_cast<unsigned char *>(address);
  for (SIZE_T i = 0; i < count; ++i) {
    cell[i] = 0x90;
  }
}

void WriteBytes(void *address, const void *data, SIZE_T count) {
  if (!MakeWritable(address, count)) {
    return;
  }
  const unsigned char *source = static_cast<const unsigned char *>(data);
  unsigned char *target = static_cast<unsigned char *>(address);
  for (SIZE_T i = 0; i < count; ++i) {
    target[i] = source[i];
  }
}

bool WaitForByte(volatile unsigned char *address, unsigned char value) {
  MakeWritable(const_cast<unsigned char *>(address), 1);
  while (*address != value) {
    ::Sleep(1);
  }
  return MakeWritable(const_cast<unsigned char *>(address), 1);
}

}  // namespace samp::game
