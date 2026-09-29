#include "samp/client/spawn.h"
#include "samp/client/controller.h"
#include "samp/game/memory.h"
#include "samp/util/logger.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace samp::client {
namespace {

constexpr uintptr_t kLocalPlayerPointerAddress = 0xB7CD98;
constexpr uintptr_t kPedPoolPointerAddress = 0xB74490;
constexpr uintptr_t kPedPoolGetByHandleAddress = 0x404910;
constexpr uintptr_t kProcessOneScriptCommandAddress = 0x469EB0;
constexpr unsigned kLocalPlayerIndex = 1;
constexpr size_t kScriptLocalVariablesOffset = 0x3C;
constexpr size_t kScriptLocalVariablesSize = 0x48;

unsigned char g_script_thread[0xE0] = {};
unsigned char g_script_command[40] = {};
static_assert(kScriptLocalVariablesOffset + kScriptLocalVariablesSize <=
              sizeof(g_script_thread));
bool g_spawn_path_logged = false;
bool g_spawn_entry_logged = false;
bool g_spawn_vehicle_state_logged = false;
bool g_spawn_view_reset_logged = false;

float BitsToFloat(unsigned bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

unsigned FloatToBits(float value) {
  unsigned bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

void *LocalPlayer() {
  return *reinterpret_cast<void *volatile *>(kLocalPlayerPointerAddress);
}

void InvokeScriptCommand(unsigned short opcode, const unsigned char *argument_types,
                         const unsigned *argument_values, size_t argument_count,
                         bool write_log = true) {
  if (write_log) {
    samp::util::WriteLogNumber("local spawn script opcode", opcode);
  }
  size_t offset = 0;
  std::memcpy(g_script_command + offset, &opcode, sizeof(opcode));
  offset += sizeof(opcode);
  for (size_t index = 0; index < argument_count; ++index) {
    g_script_command[offset++] = argument_types[index];
    std::memcpy(g_script_command + offset, &argument_values[index], sizeof(unsigned));
    offset += sizeof(unsigned);
  }
  g_script_command[offset] = 0;
  std::memset(g_script_thread + kScriptLocalVariablesOffset, 0,
              kScriptLocalVariablesSize);
  unsigned char *command = g_script_command;
  std::memcpy(g_script_thread + 0x14, &command, sizeof(command));
  using ProcessOneCommand = void(__thiscall *)(void *);
  reinterpret_cast<ProcessOneCommand>(kProcessOneScriptCommandAddress)(g_script_thread);
  if (write_log) {
    samp::util::WriteLogNumber("local spawn script opcode completed", opcode);
  }
}

void InvokeFloatCommand(unsigned short opcode, const float *arguments, size_t argument_count) {
  unsigned char argument_types[6] = {};
  unsigned argument_values[6] = {};
  for (size_t index = 0; index < argument_count; ++index) {
    argument_types[index] = 6;
    argument_values[index] = FloatToBits(arguments[index]);
  }
  InvokeScriptCommand(opcode, argument_types, argument_values, argument_count);
}

std::size_t RequestServerModels(const std::array<char, kServerModelBlockSize> &model_priorities,
                                std::array<bool, kServerModelBlockSize> &loaded_models) {
  std::array<std::uint8_t, kServerModelBlockSize> priorities{};
  std::array<std::uint16_t, kServerModelBlockSize> model_ids{};
  loaded_models.fill(false);
  for (std::size_t index = 0; index < model_priorities.size(); ++index) {
    priorities[index] = static_cast<std::uint8_t>(model_priorities[index]);
    model_ids[index] = static_cast<std::uint16_t>(400 + index);
  }

  for (std::size_t remaining = priorities.size() - 1; remaining > 0; --remaining) {
    for (std::size_t index = 0; index < remaining; ++index) {
      if (priorities[index + 1] > priorities[index]) {
        std::swap(priorities[index], priorities[index + 1]);
        std::swap(model_ids[index], model_ids[index + 1]);
      }
    }
  }

  std::size_t requested_count = 0;
  for (std::size_t index = 0; index < 80; ++index) {
    if (priorities[index] == 0) {
      continue;
    }
    const unsigned model_id = model_ids[index];
    const unsigned char argument_types[] = {1};
    const unsigned argument_values[] = {model_id};
    InvokeScriptCommand(0x0247, argument_types, argument_values, 1, false);
    InvokeScriptCommand(0x038B, nullptr, nullptr, 0, false);
    loaded_models[model_id - 400] = true;
    ++requested_count;
  }
  return requested_count;
}

void PlaceSpawnMarker(float x, float y, float z) {
  void *ped_pool = *reinterpret_cast<void *volatile *>(kPedPoolPointerAddress);
  if (!ped_pool) {
    return;
  }
  using GetPedByHandle = void *(__thiscall *)(void *, int);
  if (!reinterpret_cast<GetPedByHandle>(kPedPoolGetByHandleAddress)(
          ped_pool, kLocalPlayerIndex)) {
    return;
  }
  const unsigned char argument_types[] = {1, 6, 6, 6};
  const unsigned argument_values[] = {kLocalPlayerIndex, FloatToBits(x), FloatToBits(y),
                                      FloatToBits(z)};
  InvokeScriptCommand(0x0362, argument_types, argument_values, 4);
}

void SpawnWorldObject(float x, float y, float z) {
  void *player = LocalPlayer();
  if (!player) {
    if (!g_spawn_path_logged) {
      samp::util::WriteLogLine("local spawn player pointer is null");
      g_spawn_path_logged = true;
    }
    return;
  }
  auto *vtable = *reinterpret_cast<uintptr_t **>(player);
  if (!vtable || reinterpret_cast<uintptr_t>(vtable) == 0x863C40) {
    if (!g_spawn_path_logged) {
      samp::util::WriteLogLine("local spawn player vtable is invalid");
      g_spawn_path_logged = true;
    }
    return;
  }
  const unsigned short model_id =
      *reinterpret_cast<volatile unsigned short *>(static_cast<unsigned char *>(player) + 0x22);
  if (model_id == 538 || model_id == 537 || model_id == 449) {
    const unsigned char argument_types[] = {1, 6, 6, 6};
    const unsigned argument_values[] = {kLocalPlayerIndex, FloatToBits(x), FloatToBits(y),
                                        FloatToBits(z)};
    InvokeScriptCommand(0x07C7, argument_types, argument_values, 4);
    return;
  }
  using SpawnFunction = int(__thiscall *)(void *, float, float, float, int);
  reinterpret_cast<SpawnFunction>(vtable[14])(player, x, y, z, 0);
  if (!g_spawn_path_logged) {
    samp::util::WriteLogLine("local player spawn invoked");
    g_spawn_path_logged = true;
  }
}

void InvokeViewBounds(float x, float y, float z) {
  const float camera_position[] = {x, y, z, 0.0f, 0.0f, 0.0f};
  InvokeScriptCommand(0x0925, nullptr, nullptr, 0);
  InvokeFloatCommand(0x015F, camera_position, 6);
}

void InvokeViewZoom(float x, float y, float z, int mode) {
  const unsigned char argument_types[] = {6, 6, 6, 1};
  const unsigned argument_values[] = {FloatToBits(x), FloatToBits(y), FloatToBits(z),
                                       static_cast<unsigned>(mode)};
  InvokeScriptCommand(0x0925, nullptr, nullptr, 0);
  InvokeScriptCommand(0x0160, argument_types, argument_values, 4);
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
  samp::game::WriteByte(reinterpret_cast<void *>(0xBAA3FB), 1);
}

bool LocalPlayerInVehicle() {
  void *player = LocalPlayer();
  if (!player) {
    return false;
  }
  return (*reinterpret_cast<volatile unsigned *>(
              static_cast<unsigned char *>(player) + 1132) & 0x100) != 0;
}

}  // namespace

void ResetSpawnView(void *status) {
  const unsigned char argument_types[] = {1};
  const unsigned argument_values[] = {0};
  const bool write_log = !g_spawn_view_reset_logged;
  if (status) {
    *static_cast<unsigned *>(status) = 0;
  }
  InvokeScriptCommand(0x02EB, nullptr, nullptr, 0, write_log);
  InvokeScriptCommand(0x0930, argument_types, argument_values, 1, write_log);
  InvokeScriptCommand(0x0925, nullptr, nullptr, 0, write_log);
  if (status) {
    *static_cast<unsigned *>(status) = 0;
  }
  InvokeScriptCommand(0x0373, nullptr, nullptr, 0, write_log);
  InvokeScriptCommand(0x02EB, nullptr, nullptr, 0, write_log);
  g_spawn_view_reset_logged = true;
}

void DoSpawn() {
  if (!g_spawn_entry_logged) {
    samp::util::WriteLogLine("local do spawn entered");
    g_spawn_entry_logged = true;
  }
  bool in_vehicle = LocalPlayerInVehicle();
  if (!g_spawn_vehicle_state_logged) {
    samp::util::WriteLogNumber("local spawn in vehicle", in_vehicle ? 1 : 0);
    g_spawn_vehicle_state_logged = true;
  }
  if (in_vehicle) {
    PlaceSpawnMarker(BitsToFloat(0x4488ACCD), BitsToFloat(0xC4FE9000), BitsToFloat(0x42A56BD4));
  } else {
    SpawnWorldObject(BitsToFloat(0x448DA19D), BitsToFloat(0xC4FECCE9), BitsToFloat(0x428A3333));
  }
  samp::util::WriteLogLine("local spawn position step completed");
  SetSpawnViewBounds();
  samp::util::WriteLogLine("local spawn view bounds completed");
  SetSpawnViewZoom();
  samp::util::WriteLogLine("local spawn view zoom completed");
  SetSpawnWeather(1);
  samp::util::WriteLogLine("local spawn weather completed");
  SetSpawnUnpaused();
  samp::util::WriteLogLine("local do spawn completed");
}

void SetSpawnViewBounds() {
  InvokeViewBounds(BitsToFloat(0x4488A000), BitsToFloat(0xC4FE8000), BitsToFloat(0x42B40000));
}

void SetSpawnViewZoom() {
  InvokeViewZoom(BitsToFloat(0x43C00000), BitsToFloat(0xC4C2A000), BitsToFloat(0x41A00000), 2);
}

void ApplyServerWeather(int weather) {
  SetSpawnWeather(weather);
}

std::size_t ApplyServerModelSettings(
    const std::array<char, kServerModelBlockSize> &model_priorities,
    std::array<bool, kServerModelBlockSize> &loaded_models) {
  return RequestServerModels(model_priorities, loaded_models);
}

}  // namespace samp::client
