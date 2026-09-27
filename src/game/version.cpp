#include "samp/game/version.h"
#include "samp/game/memory.h"
#include "samp/util/logger.h"

#include <windows.h>

namespace samp::game {
namespace {

int g_version_index = 0;
unsigned g_total_ram_mb = 0;
unsigned g_streaming_memory = 0;

volatile unsigned char *SignatureCell() {
  return reinterpret_cast<volatile unsigned char *>(0x747483);
}

volatile unsigned char *SecondSignatureCell() {
  return reinterpret_cast<volatile unsigned char *>(0x7474D3);
}

volatile unsigned char *ShutdownFlag() {
  return reinterpret_cast<volatile unsigned char *>(0xBA6831);
}

volatile unsigned char *BootState() {
  return reinterpret_cast<volatile unsigned char *>(0xC8D4C0);
}

void PatchSignatureNop(volatile unsigned char *cell) {
  FillNops(const_cast<unsigned char *>(cell), 6);
}

unsigned StreamingMemoryForRam(unsigned ram_mb) {
  if (ram_mb > 4000) {
    return 0x40000000;
  }
  if (ram_mb > 2000) {
    return 0x20000000;
  }
  if (ram_mb > 1000) {
    return 0x10000000;
  }
  if (ram_mb > 500) {
    return 0x8000000;
  }
  return 0x6000000;
}

}  // namespace

int GameVersionIndex() {
  return g_version_index;
}

int DetectGameVersion() {
  volatile unsigned char *signature = SignatureCell();
  int spins = 0;
  while (*signature != 0x89 && *signature != 0xC8) {
    if (*ShutdownFlag() == 1) {
      return 0;
    }
    ::Sleep(10);
    if (++spins > 6000) {
      return 0;
    }
    signature = SignatureCell();
  }
  if (*SignatureCell() == 0x89) {
    g_version_index = 1;
    PatchSignatureNop(SignatureCell());
  } else if (*SignatureCell() == 0xC8) {
    g_version_index = 2;
    PatchSignatureNop(SecondSignatureCell());
  } else if (g_version_index == 1) {
    PatchSignatureNop(SignatureCell());
  } else if (g_version_index == 2) {
    PatchSignatureNop(SecondSignatureCell());
  } else {
    return 0;
  }
  *BootState() = 5;
  WriteBytes(reinterpret_cast<void *>(0x866CD8), "title", 6);
  WriteBytes(reinterpret_cast<void *>(0x866CCC), "title", 6);
  FillNops(reinterpret_cast<void *>(0x745B87), 0x44);
  WriteWord(reinterpret_cast<void *>(0x7459E1), 0x9090);
  WaitForByte(reinterpret_cast<volatile unsigned char *>(0x561872), 133);
  WriteByte(reinterpret_cast<void *>(0x561872), 0x33);
  WriteByte(reinterpret_cast<void *>(0x561873), 0xC0);
  FillNops(reinterpret_cast<void *>(0x561874), 27);
  MEMORYSTATUSEX status = {};
  status.dwLength = sizeof(status);
  ::GlobalMemoryStatusEx(&status);
  g_total_ram_mb = static_cast<unsigned>(status.ullTotalPhys >> 20);
  g_streaming_memory = StreamingMemoryForRam(g_total_ram_mb);
  WriteDword(reinterpret_cast<void *>(0x5B8E6A), g_streaming_memory);
  WaitForByte(reinterpret_cast<volatile unsigned char *>(0x4083C0), 184);
  WriteByte(reinterpret_cast<void *>(0x4083C0), 0xC3);
  WriteDword(reinterpret_cast<void *>(0x590099), 0x90909090);
  WriteByte(reinterpret_cast<void *>(0x59009D), 0x90);
  WriteByte(reinterpret_cast<void *>(0x53E94C), 2);
  WriteDword(reinterpret_cast<void *>(0x731F60), 20000);
  samp::util::WriteLogNumber("game version index", static_cast<unsigned>(g_version_index));
  samp::util::WriteLogNumber("ram mb", g_total_ram_mb);
  samp::util::WriteLogNumber("streaming memory", g_streaming_memory);
  return 1;
}

}  // namespace samp::game
