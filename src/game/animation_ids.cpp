#include "samp/game/animation_ids.h"

#include <cstddef>
#include <cstring>

namespace samp::game {
namespace {

const char *g_animation_names[] = {
#include "anim_names.inc"
};

int g_animation_ids[1812] = {};

int LookupAssetId(const char *name) {
  using Lookup = int(__cdecl *)(const char *, int);
  return reinterpret_cast<Lookup>(0x53CF30)(name, 0);
}

}  // namespace

void BuildAnimationIdTable() {
  for (size_t i = 0; i < 1812; ++i) {
    const char *entry = g_animation_names[i];
    size_t prefix = 0;
    while (entry[prefix] != 0 && entry[prefix] != ':') {
      ++prefix;
    }
    if (entry[prefix] == 0) {
      continue;
    }
    char asset[64] = {};
    size_t length = 0;
    const char *name = entry + prefix + 1;
    while (name[length] != 0 && length + 1 < sizeof(asset)) {
      asset[length] = name[length];
      ++length;
    }
    asset[length] = 0;
    g_animation_ids[i] = LookupAssetId(asset);
  }
}

int AnimationId(int index) {
  if (index < 0 || static_cast<size_t>(index) >= 1812) {
    return 0;
  }
  return g_animation_ids[index];
}

}  // namespace samp::game
