#include "samp/player/player_colors.h"

#include <cstddef>
#include <cstring>

namespace samp::player {
namespace {

const unsigned char g_default_colors[4000] = {
#include "player_colors.inc"
};

struct ColorPools {
  unsigned colors[1000] = {};
  unsigned char cache_a[0x134] = {};
  unsigned char gap[4] = {};
  unsigned char cache_b[0xFCA8] = {};
};

ColorPools g_pools;

}  // namespace

void InitPlayerColors() {
  std::memcpy(g_pools.colors, g_default_colors, sizeof(g_pools.colors));
}

void ResetPlayerColors() {
  std::memset(g_pools.cache_a, 0, sizeof(g_pools.cache_a));
  std::memset(g_pools.cache_b, 0, sizeof(g_pools.cache_b));
}

unsigned PlayerColorByID(unsigned id) {
  if (id == 0x3EC) {
    return static_cast<unsigned>(-1985690560);
  }
  if (id == 0x3ED) {
    return static_cast<unsigned>(-1442840321);
  }
  if (id == 0x3EE) {
    return static_cast<unsigned>(-490707969);
  }
  if (id < 0xFA0) {
    const unsigned *view = reinterpret_cast<const unsigned *>(&g_pools);
    return view[id];
  }
  return id;
}

}  // namespace samp::player
