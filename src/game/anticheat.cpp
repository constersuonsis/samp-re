#include "samp/game/anticheat.h"

#include "samp/client/session.h"
#include "samp/client/tick.h"
#include "samp/game/memory.h"

#include <cstdlib>

namespace samp::game {
namespace {

volatile unsigned *MemoryDword(unsigned address) {
  return reinterpret_cast<volatile unsigned *>(address);
}

volatile unsigned char *MemoryByte(unsigned address) {
  return reinterpret_cast<volatile unsigned char *>(address);
}

unsigned ComputeChecksum(const void *address, unsigned dword_count) {
  const volatile unsigned *words = static_cast<const volatile unsigned *>(address);
  unsigned checksum = 0;
  for (unsigned index = 0; index < dword_count; ++index) {
    checksum ^= words[index];
  }
  return checksum;
}

void *GetGtaPlayer() {
  unsigned pool = *MemoryDword(0xB74494);
  return pool ? reinterpret_cast<void *>(*MemoryDword(pool)) : nullptr;
}

void *GetGtaVehicle() {
  unsigned pool = *MemoryDword(0xB74490);
  return pool ? reinterpret_cast<void *>(*MemoryDword(pool)) : nullptr;
}

void ResetWeather() {
  *MemoryDword(0xB73424) = 0;
  *MemoryDword(0xB73428) = 0;
  reinterpret_cast<void(__cdecl *)()>(0x541BD0)();
}

void UpdateWeather() {
  reinterpret_cast<void(__cdecl *)()>(0x541DD0)();
}

void PatchWeaponModel() {
  const unsigned char call_patch[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
  const unsigned char branch_patch[4] = {0x33, 0xC0, 0x0F, 0x84};
  WriteBytes(reinterpret_cast<void *>(0x53F417), call_patch, sizeof(call_patch));
  WriteBytes(reinterpret_cast<void *>(0x53F41F), branch_patch, sizeof(branch_patch));
}

void RecordFailure(unsigned id) {
  ++*MemoryDword(0x1026E918);
  if (CrashHandlerProbe(static_cast<int>(id)) != 69) {
    ::exit(0);
  }
}

}

void SetAntiCheatFlag() {
  *MemoryDword(0x1014FAF4) = 1;
}

void ValidateMemory() {
  volatile unsigned *player_checksum = MemoryDword(0x1014FAE0);
  volatile unsigned *vehicle_checksum = MemoryDword(0x1014FAE4);
  volatile unsigned *validation_pending = MemoryDword(0x1014FAEC);
  volatile unsigned *modified_memory_count = MemoryDword(0x1014FAE8);
  volatile unsigned *failure_count = MemoryDword(0x1026E918);
  volatile unsigned char *suppress_count = MemoryByte(0x1014FAF0);

  if (!*validation_pending) {
    void *player = GetGtaPlayer();
    *player_checksum = player ? ComputeChecksum(player, 2000) : 0;
    void *vehicle = GetGtaVehicle();
    *vehicle_checksum = vehicle ? ComputeChecksum(vehicle, 500) : 0;
    *validation_pending = 1;
    return;
  }

  void *player = GetGtaPlayer();
  unsigned current_player_checksum = player ? ComputeChecksum(player, 2000) : 0;
  void *vehicle = GetGtaVehicle();
  unsigned current_vehicle_checksum = vehicle ? ComputeChecksum(vehicle, 500) : 0;

  if (*player_checksum && *player_checksum != current_player_checksum) {
    ++*failure_count;
    RecordFailure(12);
  }
  if (*player_checksum && !*suppress_count) {
    ++*modified_memory_count;
  }
  if (*vehicle_checksum && *vehicle_checksum != current_vehicle_checksum) {
    ++*failure_count;
    RecordFailure(11);
  }
  if (*vehicle_checksum && !*suppress_count) {
    ++*modified_memory_count;
  }
  *validation_pending = 0;
}

int CrashHandlerProbe(int id) {
  if (!samp::client::HasClient() || !samp::client::SessionDevice()) {
    return 69;
  }

  const unsigned char call_patch[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
  WriteBytes(reinterpret_cast<void *>(0x541DF5), call_patch, sizeof(call_patch));
  PatchWeaponModel();
  ResetWeather();
  UpdateWeather();
  WriteByte(reinterpret_cast<void *>(0x6194A0), 0xC3);
  ++*MemoryDword(0x1026E918);
  (void)id;
  return 69;
}

int AntiCheatCheck(bool entering) {
  volatile unsigned *guard = MemoryDword(0x10117480);
  volatile unsigned *modified_memory_count = MemoryDword(0x1014FAE8);
  volatile unsigned *last_enter_count = MemoryDword(0x1026E934);
  volatile unsigned *last_exit_count = MemoryDword(0x1026E930);
  volatile unsigned *tick_count = MemoryDword(0x1026E938);
  volatile unsigned *failure_count = MemoryDword(0x1026E918);

  if (*guard == 69) {
    *last_enter_count = *modified_memory_count;
    *last_exit_count = *modified_memory_count;
    *guard = 70;
    return static_cast<int>(++*tick_count);
  }

  if (entering) {
    if (*modified_memory_count <= *last_enter_count && CrashHandlerProbe(59) != 69) {
      ::exit(0);
    }
    if (*last_exit_count >= *modified_memory_count && CrashHandlerProbe(60) != 69) {
      ::exit(0);
    }
    *last_exit_count = *modified_memory_count;
  }

  if (*failure_count > 5 && CrashHandlerProbe(65) != 69) {
    ::exit(0);
  }
  return static_cast<int>(++*tick_count);
}

}
