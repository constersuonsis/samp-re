#pragma once

#include <windows.h>

namespace samp::game {

bool MakeWritable(void *address, SIZE_T size);
void WriteByte(void *address, unsigned char value);
void WriteWord(void *address, unsigned short value);
void WriteDword(void *address, unsigned int value);
void FillNops(void *address, SIZE_T count);
void WriteBytes(void *address, const void *data, SIZE_T count);
bool WaitForByte(volatile unsigned char *address, unsigned char value);

}  // namespace samp::game
