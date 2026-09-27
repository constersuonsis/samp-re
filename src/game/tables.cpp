#include "samp/game/tables.h"
#include "samp/game/memory.h"
#include "samp/game/version.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace samp::game {
namespace {

unsigned char g_model_pool[0x27100] = {};
unsigned char g_pickup_pool[319 * 68] = {};
unsigned char g_second_pickup_pool[11760 * 32] = {};
unsigned char g_model_reference_copy[0x400] = {};
unsigned char g_stream_extension[0x4000] = {};

const unsigned char g_color_template[0x400] = {
#include "color_template.inc"
};

const uintptr_t g_model_slots_v1[14] = {
    0x5DC7AA, 0x41A85D, 0x41A864, 0x408259, 0x711B32, 0x699CF8, 0x4092EC,
    0x40914E, 0x408702, 0x564220, 0x564172, 0x563845, 0x84E9C2, 0x85652D,
};

const uintptr_t g_model_slots_v2[14] = {
    0x5DC7AA, 0x41A85D, 0x41A864, 0x408261, 0x711B32, 0x699CF8, 0x4092EC,
    0x40914E, 0x408702, 0x564220, 0x564172, 0x563845, 0x84EA02, 0x85656D,
};

const uintptr_t g_model_blocks_v1[56] = {
    0x40D68C, 0x5664D7, 0x566586, 0x408706, 0x56B3B1, 0x56AD91, 0x56A85F, 0x5675FA,
    0x56CD84, 0x56CC79, 0x56CB51, 0x56CA4A, 0x56C664, 0x56C569, 0x56C445, 0x56C341,
    0x56BD46, 0x56BC53, 0x56BE56, 0x56A940, 0x567735, 0x546738, 0x54BB23, 0x6E31AA,
    0x40DC29, 0x534A09, 0x534D6B, 0x564B59, 0x564DA9, 0x67FF5D, 0x568CB9, 0x568EFB,
    0x569F57, 0x569537, 0x569127, 0x56B4B5, 0x56B594, 0x56B2C3, 0x56AF74, 0x56AE95,
    0x56BF4F, 0x56ACA3, 0x56A766, 0x56A685, 0x70B9BA, 0x56479D, 0x70ACB2, 0x6063C7,
    0x699CFE, 0x41A861, 0x40E061, 0x40DF5E, 0x40DDCE, 0x40DB0E, 0x40D98C, 0x1566855,
};

const uintptr_t g_model_blocks_v2[56] = {
    0x40D68C, 0x5664D7, 0x566586, 0x408706, 0x56B3B1, 0x56AD91, 0x56A85F, 0x5675FA,
    0x56CD84, 0x56CC79, 0x56CB51, 0x56CA4A, 0x56C664, 0x56C569, 0x56C445, 0x56C341,
    0x56BD46, 0x56BC53, 0x56BE56, 0x56A940, 0x567735, 0x546738, 0x54BB23, 0x6E31AA,
    0x40DC29, 0x534A09, 0x534D6B, 0x564B59, 0x564DA9, 0x67FF5D, 0x568CB9, 0x568EFB,
    0x569F57, 0x569537, 0x569127, 0x56B4B5, 0x56B594, 0x56B2C3, 0x56AF74, 0x56AE95,
    0x56BF4F, 0x56ACA3, 0x56A766, 0x56A685, 0x70B9BA, 0x56479D, 0x70ACB2, 0x6063C7,
    0x699CFE, 0x41A861, 0x40E061, 0x40DF5E, 0x40DDCE, 0x40DB0E, 0x40D98C, 0x1566845,
};

const uintptr_t g_model_fixups[12] = {
    0x4091C5, 0x409367, 0x40D9C5, 0x40DB47, 0x40DC61, 0x40DE07,
    0x40DF97, 0x40E09A, 0x534A98, 0x534DFA, 0x71CDB0, 0x5634A6,
};

const uintptr_t g_pool_fixups[4] = {
    0x5634A6, 0x5638DF, 0x56420F, 0x564283,
};

const uintptr_t g_pickup_slots[14] = {
    0x4C63F2, 0x4C662D, 0x4C6822, 0x4C6829, 0x4C6877, 0x4C6881, 0x4C6890,
    0x4C68A5, 0x4C68F3, 0x4C6932, 0x4C6971, 0x4C69B0, 0x4C69EF, 0x4C6A2E,
};

void *At(uintptr_t address) {
  return reinterpret_cast<void *>(address);
}

}  // namespace

void InitModelPool() {
  std::memset(g_model_pool, 0, sizeof(g_model_pool));
  const uintptr_t *slots = GameVersionIndex() == 1 ? g_model_slots_v1 : g_model_slots_v2;
  for (int i = 0; i < 14; ++i) {
    WriteDword(At(slots[i]), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_pool)));
  }
  const uintptr_t *blocks = GameVersionIndex() == 1 ? g_model_blocks_v1 : g_model_blocks_v2;
  for (int i = 0; i < 56; ++i) {
    WriteDword(At(blocks[i] + 3), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_pool)));
  }
  for (uintptr_t slot : g_model_fixups) {
    WriteDword(At(slot + 3), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_pool + 4)));
  }
  for (uintptr_t slot : g_pool_fixups) {
    WriteDword(At(slot), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_pickup_pool)));
  }
  WriteDword(At(0x564DC7), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_stream_extension)));
  WriteDword(At(0x40936A), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_pool + 4)));
  MakeWritable(At(0xB7D0B8), 0x1C200);
  std::memset(At(0xB7D0B8), 0, 0x1C200);
}

void InitPickupPool() {
  for (int i = 0; i < 319; ++i) {
    unsigned char *entry = g_pickup_pool + static_cast<size_t>(i) * 68;
    *reinterpret_cast<unsigned *>(entry) = 8764864;
    std::memset(entry + 4, 0, 64);
  }
  WriteDword(At(0x4C67AD), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_pickup_pool)));
}

void InitSecondPickupPool() {
  for (int i = 0; i < 11760; ++i) {
    unsigned char *entry = g_second_pickup_pool + static_cast<size_t>(i) * 32;
    *reinterpret_cast<unsigned *>(entry) = 8764400;
    std::memset(entry + 4, 0, 28);
  }
  for (uintptr_t slot : g_pickup_slots) {
    WriteDword(At(slot), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_second_pickup_pool)));
  }
}

void PatchModelReferences() {
  std::memcpy(g_model_reference_copy, g_color_template, sizeof(g_model_reference_copy));
  WriteDword(At(0x44B1C1), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy)));
  WriteDword(At(0x4C8390), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy)));
  WriteDword(At(0x4C8399), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy + 1)));
  WriteDword(At(0x4C83A3), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy + 2)));
  WriteDword(At(0x5817CC), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy)));
  WriteDword(At(0x582176), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy)));
  WriteDword(At(0x6A6FFA), static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_model_reference_copy)));
}

}  // namespace samp::game
