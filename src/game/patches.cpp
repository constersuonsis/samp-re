#include "samp/game/patches.h"
#include "samp/game/limits.h"
#include "samp/game/memory.h"
#include "samp/game/tables.h"
#include "samp/game/version.h"
#include "samp/util/logger.h"

#include <windows.h>

namespace samp::game {
namespace {

unsigned g_model_ref_a = 0;
unsigned g_model_ref_b = 0;
unsigned g_table_a = 0x40000000;
unsigned g_table_b = 0x461C4000;
unsigned g_table_c = 0xC69C4000;
unsigned g_table_d = 0x469C4000;
unsigned g_table_e = 0;
unsigned char g_hook_tail[7] = {0xB8, 0x89, 0x8F, 0x6F, 0x00, 0xFF, 0xE0};

void PatchCallSite(void *address) {
  WriteDword(address, 0x90909090);
  WriteByte(static_cast<unsigned char *>(address) + 4, 0x90);
}

void PatchCallSite6(void *address) {
  WriteDword(address, 0x90909090);
  WriteWord(static_cast<unsigned char *>(address) + 4, 0x9090);
}

int DeleteStaleObject() {
  return 0;
}

}  // namespace

void ApplyAllPatches() {
  if (GameVersionIndex() == 1) {
    g_model_ref_a = 8315552;
    g_model_ref_b = 8315561;
  } else {
    g_model_ref_a = 8315616;
    g_model_ref_b = 8315625;
  }
  InitModelPool();
  InitPickupPool();
  ::samp::game::PatchModelReferences();
  InitSecondPickupPool();
  ::samp::game::PatchLimits();
  samp::util::WriteLogLine("model and pickup tables ready");
  PatchCallSite(reinterpret_cast<void *>(0x4DF7E2));
  WriteByte(reinterpret_cast<void *>(0x5B8FE4), 127);
  PatchCallSite(reinterpret_cast<void *>(0x5720A5));
  PatchCallSite(reinterpret_cast<void *>(0x53C159));
  PatchCallSite(reinterpret_cast<void *>(0x53C136));
  if (GameVersionIndex() == 1) {
    PatchCallSite(reinterpret_cast<void *>(0x748063));
  } else {
    PatchCallSite(reinterpret_cast<void *>(0x7480B3));
  }
  PatchCallSite(reinterpret_cast<void *>(0x58FBE9));
  WriteByte(reinterpret_cast<void *>(0x86D1EC), 0);
  PatchCallSite(reinterpret_cast<void *>(0x53C090));
  WriteDword(reinterpret_cast<void *>(0x440833), 0x90909090);
  WriteDword(reinterpret_cast<void *>(0x440837), 0x90909090);
  FillNops(reinterpret_cast<void *>(0x53EA08), 10);
  WriteByte(reinterpret_cast<void *>(0x71162C), 80);
  FillNops(reinterpret_cast<void *>(0x609C08), 0x24);
  WriteWord(reinterpret_cast<void *>(0x609C2C), 0x9090);
  WriteByte(reinterpret_cast<void *>(0x609C2E), 0x90);
  FillNops(reinterpret_cast<void *>(0x56E0FA), 18);
  WriteWord(reinterpret_cast<void *>(0x6BC9EB), 0x9090);
  WriteByte(reinterpret_cast<void *>(0x4BC6C1), 0xB0);
  WriteByte(reinterpret_cast<void *>(0x4BC6C2), 0x00);
  WriteByte(reinterpret_cast<void *>(0x4BC6C3), 0x90);
  WriteDword(reinterpret_cast<void *>(0x44AC75), static_cast<unsigned>(-1869610576));
  WriteByte(reinterpret_cast<void *>(0x44AC79), 0x90);
  WriteByte(reinterpret_cast<void *>(0x5B47B0), 0xC3);
  FillNops(reinterpret_cast<void *>(0x5E63A6), 19);
  FillNops(reinterpret_cast<void *>(0x621AEA), 12);
  FillNops(reinterpret_cast<void *>(0x62D331), 11);
  FillNops(reinterpret_cast<void *>(0x741FFF), 27);
  PatchCallSite(reinterpret_cast<void *>(0x704E8A));
  WriteByte(reinterpret_cast<void *>(0x4090A0), 0xC3);
  PatchCallSite(reinterpret_cast<void *>(0x441482));
  WriteByte(reinterpret_cast<void *>(0x588BE0), 0xC3);
  PatchCallSite(reinterpret_cast<void *>(0x53C06A));
  PatchCallSite(reinterpret_cast<void *>(0x434272));
  WriteByte(reinterpret_cast<void *>(0x60D64E), 0x84);
  FillNops(reinterpret_cast<void *>(0x542485), 11);
  WriteByte(reinterpret_cast<void *>(0x612710), 0x33);
  WriteByte(reinterpret_cast<void *>(0x612711), 0xC0);
  WriteByte(reinterpret_cast<void *>(0x612712), 0xC3);
  PatchCallSite(reinterpret_cast<void *>(0x613BA7));
  PatchCallSite6(reinterpret_cast<void *>(0x609A4E));
  WriteDword(reinterpret_cast<void *>(0x6F8CF8), 0x90909090);
  WriteByte(reinterpret_cast<void *>(0x6F8CFC), 0x90);
  WriteBytes(reinterpret_cast<void *>(0x6F8CFD), g_hook_tail, sizeof(g_hook_tail));
  PatchCallSite(reinterpret_cast<void *>(0x53C1C1));
  WriteDword(reinterpret_cast<void *>(0x540040), 0x90909090);
  WriteWord(reinterpret_cast<void *>(0x540044), 0x9090);
  WriteByte(reinterpret_cast<void *>(0x540046), 0x85);
  WriteByte(reinterpret_cast<void *>(0x540047), 0xC9);
  WriteByte(reinterpret_cast<void *>(0x540048), 0x74);
  PatchCallSite(reinterpret_cast<void *>(0x56E5AD));
  MakeWritable(reinterpret_cast<void *>(0x541DF5), 5);
  PatchCallSite(reinterpret_cast<void *>(0x609CB4));
  FillNops(reinterpret_cast<void *>(0x60F2C4), 25);
  PatchCallSite(reinterpret_cast<void *>(0x6B18F1));
  PatchCallSite(reinterpret_cast<void *>(0x6B9298));
  PatchCallSite(reinterpret_cast<void *>(0x6F1793));
  PatchCallSite(reinterpret_cast<void *>(0x6F86B6));
  WriteByte(reinterpret_cast<void *>(0x47C477), 0xEB);
  WriteByte(reinterpret_cast<void *>(0x53A984), 0xEB);
  WriteByte(reinterpret_cast<void *>(0x53A985), 0x77);
  WriteDword(reinterpret_cast<void *>(0x60F289), 0x90909090);
  WriteDword(reinterpret_cast<void *>(0x60F28D), 0x90909090);
  FillNops(reinterpret_cast<void *>(0x60F29D), 19);
  WriteByte(reinterpret_cast<void *>(0x58DB5F), 0xBD);
  WriteByte(reinterpret_cast<void *>(0x58DB60), 0x00);
  WriteByte(reinterpret_cast<void *>(0x58DB61), 0x00);
  WriteByte(reinterpret_cast<void *>(0x58DB62), 0x00);
  WriteByte(reinterpret_cast<void *>(0x58DB63), 0x00);
  WriteByte(reinterpret_cast<void *>(0x58DB64), 0x90);
  WriteByte(reinterpret_cast<void *>(0x58DB65), 0x90);
  WriteByte(reinterpret_cast<void *>(0x58DB66), 0x90);
  WriteByte(reinterpret_cast<void *>(0x58DB67), 0x90);
  PatchCallSite(reinterpret_cast<void *>(0x575B0E));
  PatchCallSite6(reinterpret_cast<void *>(0x63ADC8));
  FillNops(reinterpret_cast<void *>(0x58FC2E), 30);
  WriteByte(reinterpret_cast<void *>(0x5E1E70), 0x33);
  WriteByte(reinterpret_cast<void *>(0x5E1E71), 0xC0);
  MakeWritable(reinterpret_cast<void *>(static_cast<uintptr_t>(0x6A8500)), 0x77);
  FillNops(reinterpret_cast<void *>(0x6A8500), 0x74);
  WriteWord(reinterpret_cast<void *>(0x6A8574), 0xC3C3);
  WriteByte(reinterpret_cast<void *>(0x6A8576), 0xC3);
  PatchCallSite(reinterpret_cast<void *>(0x593D7C));
  if (GameVersionIndex() == 1) {
    WriteByte(reinterpret_cast<void *>(0x7469AF), 0x33);
    WriteByte(reinterpret_cast<void *>(0x7469B0), 0xC0);
  } else {
    WriteByte(reinterpret_cast<void *>(0x7469FF), 0x33);
    WriteByte(reinterpret_cast<void *>(0x746A00), 0xC0);
  }
  WriteDword(reinterpret_cast<void *>(0x61E0C2), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_a)));
  WriteDword(reinterpret_cast<void *>(0x454CC9), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_b)));
  PatchCallSite(reinterpret_cast<void *>(0x5952A6));
  PatchCallSite6(reinterpret_cast<void *>(0x6884C4));
  PatchCallSite6(reinterpret_cast<void *>(0x688200));
  FillNops(reinterpret_cast<void *>(0x570535), 15);
  FillNops(reinterpret_cast<void *>(0x5E7847), 17);
  FillNops(reinterpret_cast<void *>(0x570546), 0xAC);
  WriteWord(reinterpret_cast<void *>(0x5705F2), 0x9090);
  WriteByte(reinterpret_cast<void *>(0x5705F4), 0x90);
  for (int i = 0; i < 6; ++i) {
    WriteDword(reinterpret_cast<void *>(static_cast<uintptr_t>(0x6D1874) + static_cast<uintptr_t>(i) * 4),
               7149525);
  }
  FillNops(reinterpret_cast<void *>(0x64DB49), 9);
  FillNops(reinterpret_cast<void *>(0x64BC9F), 9);
  FillNops(reinterpret_cast<void *>(0x64B872), 9);
  WriteByte(reinterpret_cast<void *>(0x5E7D62), 0x5E);
  WriteByte(reinterpret_cast<void *>(0x5E7D63), 0x5D);
  WriteByte(reinterpret_cast<void *>(0x5E7D64), 0xC2);
  WriteByte(reinterpret_cast<void *>(0x5E7D65), 0x1C);
  WriteByte(reinterpret_cast<void *>(0x5E7D66), 0x00);
  WriteDword(reinterpret_cast<void *>(0x6348F6), 0xFFFFFFFF);
  WriteDword(reinterpret_cast<void *>(0x634DE2), 0xFFFFFFFF);
  MakeWritable(reinterpret_cast<void *>(0x71A220), 0x400);
  const unsigned menu_cells[] = {0x71A223, 0x71A5D5, 0x71A247, 0x71A282, 0x71A26A, 0x71A2E5,
                                 0x71A3AC, 0x71A4B9, 0x71A53C, 0x71A37F, 0x71A518, 0x71A525,
                                 0x71A54B, 0x71A5B6, 0x71A3BB};
  for (unsigned cell : menu_cells) {
    WriteByte(reinterpret_cast<void *>(static_cast<uintptr_t>(cell)), 3);
  }
  WriteDword(reinterpret_cast<void *>(0x7361B2), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_c)));
  WriteDword(reinterpret_cast<void *>(0x7361CB), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_d)));
  WriteDword(reinterpret_cast<void *>(0x7361E0), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_c)));
  WriteDword(reinterpret_cast<void *>(0x7361F5), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&g_table_d)));
  (void)g_table_e;
  PatchCallSite(reinterpret_cast<void *>(0x524582));
  WriteByte(reinterpret_cast<void *>(0x712025), 1);
  MakeWritable(reinterpret_cast<void *>(0x858B90), 4);
  PatchCallSite(reinterpret_cast<void *>(0x53BF28));
  MakeWritable(reinterpret_cast<void *>(0x859520), 8);
  MakeWritable(reinterpret_cast<void *>(0xC17044), 8);
  FillNops(reinterpret_cast<void *>(0x4C3A5B), 10);
  MakeWritable(reinterpret_cast<void *>(0x47BF54), 4);
  DeleteStaleObject();
  samp::util::WriteLogLine("memory patches applied");
}

}  // namespace samp::game
