#include "samp/game/anticheat.h"

#include "samp/client/session.h"
#include "samp/client/tick.h"
#include "samp/game/memory.h"

#include <cstdlib>

namespace samp::game {
namespace {

unsigned g_player_checksum = 0;
unsigned g_vehicle_checksum = 0;
unsigned g_modified_memory_count = 0;
unsigned g_failure_count = 0;
unsigned g_last_enter_count = 0;
unsigned g_last_exit_count = 0;
unsigned g_tick_count = 0;
unsigned char g_validation_pending = 0;
unsigned char g_suppress_count = 0;
bool g_anti_cheat_enabled = false;

volatile unsigned *MemoryDword(unsigned address) {
  return reinterpret_cast<volatile unsigned *>(address);
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
  ++g_failure_count;
  if (CrashHandlerProbe(static_cast<int>(id)) != 69) {
    ::exit(0);
  }
}

}

void SetAntiCheatFlag() {
  g_anti_cheat_enabled = true;
}

void ValidateMemory() {
  if (!g_validation_pending) {
    void *player = GetGtaPlayer();
    g_player_checksum = player ? ComputeChecksum(player, 2000) : 0;
    void *vehicle = GetGtaVehicle();
    g_vehicle_checksum = vehicle ? ComputeChecksum(vehicle, 500) : 0;
    g_validation_pending = 1;
    return;
  }

  void *player = GetGtaPlayer();
  unsigned current_player_checksum = player ? ComputeChecksum(player, 2000) : 0;
  void *vehicle = GetGtaVehicle();
  unsigned current_vehicle_checksum = vehicle ? ComputeChecksum(vehicle, 500) : 0;

  if (g_player_checksum && g_player_checksum != current_player_checksum) {
    ++g_failure_count;
    RecordFailure(12);
  }
  if (g_player_checksum && !g_suppress_count) {
    ++g_modified_memory_count;
  }
  if (g_vehicle_checksum && g_vehicle_checksum != current_vehicle_checksum) {
    ++g_failure_count;
    RecordFailure(11);
  }
  if (g_vehicle_checksum && !g_suppress_count) {
    ++g_modified_memory_count;
  }
  g_validation_pending = 0;
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
  ++g_failure_count;
  (void)id;
  return 69;
}

int AntiCheatCheck(bool entering) {
  if (g_anti_cheat_enabled && g_tick_count == 0) {
    g_last_enter_count = g_modified_memory_count;
    g_last_exit_count = g_modified_memory_count;
    g_tick_count = 1;
    return static_cast<int>(g_tick_count);
  }

  if (entering) {
    if (g_modified_memory_count <= g_last_enter_count && CrashHandlerProbe(59) != 69) {
      ::exit(0);
    }
    if (g_last_exit_count >= g_modified_memory_count && CrashHandlerProbe(60) != 69) {
      ::exit(0);
    }
    g_last_exit_count = g_modified_memory_count;
  }

  if (g_failure_count > 5 && CrashHandlerProbe(65) != 69) {
    ::exit(0);
  }
  return static_cast<int>(++g_tick_count);
}

}
