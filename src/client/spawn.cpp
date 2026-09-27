#include "samp/client/spawn.h"
#include "samp/client/controller.h"
#include "samp/game/memory.h"

#include <cstddef>
#include <cstring>

namespace samp::client {
namespace {

float BitsToFloat(unsigned bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

void PlaceSpawnMarker(float x, float y, float z) {
  (void)x;
  (void)y;
  (void)z;
}

void SpawnWorldObject(float x, float y, float z) {
  (void)x;
  (void)y;
  (void)z;
}

void InvokeViewBounds(float x, float y, float z) {
  (void)x;
  (void)y;
  (void)z;
}

void InvokeViewZoom(float x, float y, float z, int mode) {
  (void)x;
  (void)y;
  (void)z;
  (void)mode;
}

void SetSpawnWeather(int weather) {
  auto *controller = reinterpret_cast<unsigned char *>(MainController());
  *reinterpret_cast<volatile unsigned *>(0xC81318) =
      static_cast<unsigned>(weather);
  if (!controller || *reinterpret_cast<unsigned *>(controller + 105) == 0) {
    *reinterpret_cast<volatile unsigned *>(0xC8131C) =
        static_cast<unsigned>(weather);
    *reinterpret_cast<volatile unsigned *>(0xC81320) =
        static_cast<unsigned>(weather);
  }
}

void SetSpawnUnpaused() {
  samp::game::WriteByte(reinterpret_cast<void *>(0xBA6769), 0);
  samp::game::WriteByte(reinterpret_cast<void *>(0xBAA3FB), 0);
}

bool LocalPlayerInVehicle() {
  return false;
}

}  // namespace

void DoSpawn() {
  if (LocalPlayerInVehicle()) {
    PlaceSpawnMarker(BitsToFloat(0x4488ACCD), BitsToFloat(0xC4FE9000), BitsToFloat(0x42A56BD4));
  } else {
    SpawnWorldObject(BitsToFloat(0x448DA19D), BitsToFloat(0xC4FECCE9), BitsToFloat(0x428A3333));
  }
  SetSpawnViewBounds();
  SetSpawnViewZoom();
  SetSpawnWeather(1);
  SetSpawnUnpaused();
}

void SetSpawnViewBounds() {
  InvokeViewBounds(BitsToFloat(0x4488A000), BitsToFloat(0xC4FE8000), BitsToFloat(0x42B40000));
}

void SetSpawnViewZoom() {
  InvokeViewZoom(BitsToFloat(0x43C00000), BitsToFloat(0xC4C2A000), BitsToFloat(0x41A00000), 2);
}

}  // namespace samp::client
