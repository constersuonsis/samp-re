#include "samp/game/hooks.h"
#include "samp/client/session.h"
#include "samp/game/frame.h"
#include "samp/game/memory.h"
#include "samp/game/version.h"
#include "samp/util/logger.h"

#include <cstddef>
#include <cstdint>

namespace samp::game {
namespace {

void *g_frame_callback = reinterpret_cast<void *>(RenderFrame2);

void RenderFrame() {
  samp::game::RenderFrame(nullptr);
}

void RenderPlayer() {
}

void ProcessPlayerAction() {
}

void MainTick() {
  samp::client::ProcessSessionTick();
}

void IdleSleep() {
}

void ProcessAnimation() {
}

void ProcessVehicleSync() {
}

void ProcessVehicleUpdate() {
}

void ProcessPlayerSync() {
}

void HookModelProcess() {
}

void ProcessVehicleDestroy() {
}

void ProcessObjectSync() {
}

void HookVehicleProcess() {
}

void HookVehicleProcess2() {
}

void HookVehicleProcess3() {
}

void HookVehicleProcess4() {
}

void ProcessCamera() {
}

void HandleGameProcess() {
}

void HandleGameProcess2() {
}

void HandleVehicleEnter() {
}

void StartGame() {
}

void SetPlayerColor() {
}

void ShutdownGame() {
}

void HandleDamage() {
}

void ProcessNetStats() {
}

void CheckProcess() {
}

void ProcessWeaponPickup() {
}

void ProcessObjectUpdate() {
}

void FilterVehicleSync() {
}

void GuardLocalPlayerAction() {
}

void HandleSpawnRequest() {
}

void SwapPlayerSettings() {
}

void FilterVehicleDelete() {
}

void InvokeModelRoutine() {
}

void ApplyWeaponPreset() {
}

void ProcessPlayerSyncAll() {
}

void ProcessGameLogic() {
}

void ProcessPlayerUpdate() {
}

void CreatePlayerObject() {
}

void SetPlayerSkin() {
}

void ResetHealth() {
}

void GetNetworkData() {
}

void InitProcess() {
}

void SetProcessParam() {
}

void GetPlayerPointer() {
}

void ProcessCameraShake() {
}

void HookObjectUpdate() {
}

void ProcessObjectCreate() {
}

void ProcessObjectDelete() {
}

void ProcessMission() {
}

void ProcessLineOfSight() {
}

void CheckLineOfSight() {
}

void IsVehicleValid() {
}

void CheckVehicleSync() {
}

unsigned g_sync_relay = 0;
unsigned g_process_slot_value = 0;

using Handler = void (*)();

struct PointerRedirect {
  uintptr_t slot;
  Handler handler;
};

const PointerRedirect g_pointer_redirects[] = {
    {0x86D190, ProcessVehicleSync}, {0x86C0D0, ProcessVehicleUpdate}, {0x86D744, SwapPlayerSettings},
    {0x86D194, FilterVehicleDelete}, {0x871148, ProcessPlayerSync},   {0x8721C8, ProcessPlayerSync},
    {0x871388, ProcessPlayerSync},  {0x871970, ProcessPlayerSync},   {0x8716A8, ProcessPlayerSync},
    {0x871550, ProcessPlayerSync},  {0x871800, ProcessPlayerSync},   {0x871B10, ProcessPlayerSync},
    {0x872398, ProcessPlayerSync},  {0x871C50, ProcessPlayerSync},   {0x866FA8, HookModelProcess},
    {0x866F7C, InvokeModelRoutine},    {0x866F80, ProcessVehicleDestroy}, {0x8585E8, ProcessVehicleDestroy},
    {0x871218, ProcessObjectSync},  {0x871778, ProcessObjectSync},   {0x8718D0, ProcessObjectSync},
    {0x871A40, ProcessObjectSync},  {0x871BE0, ProcessObjectSync},   {0x871178, HookVehicleProcess},
    {0x8716D8, HookVehicleProcess}, {0x8719A0, HookVehicleProcess},  {0x871B40, HookVehicleProcess},
    {0x8713B8, HookVehicleProcess2}, {0x871580, HookVehicleProcess2}, {0x871830, HookVehicleProcess3},
    {0x8721F8, HookVehicleProcess4}, {0x872A74, ProcessCamera},
};

struct CodeHook {
  uintptr_t target;
  uintptr_t slot;
  Handler handler;
  unsigned char trampoline[10];
  unsigned char size;
};

const CodeHook g_code_hooks[] = {
    {0x4D3AA0, 0x4D3934, HandleGameProcess, {0xFF, 0x25, 0x34, 0x39, 0x4D, 0x00, 0x90, 0x90, 0x90, 0x90}, 10},
    {0x4D4610, 0x4D4609, HandleGameProcess2, {0xFF, 0x25, 0x09, 0x46, 0x4D, 0x00, 0x90}, 7},
    {0x6402F0, 0x6919BB, HandleVehicleEnter, {0xFF, 0x25, 0xBB, 0x19, 0x69, 0x00, 0x90}, 7},
    {0x63B8C0, 0x63B8BA, HandleSpawnRequest, {0xFF, 0x25, 0xBA, 0xB8, 0x63, 0x00, 0x90}, 7},
    {0x438576, 0x4385AA, StartGame, {0xFF, 0x25, 0xAA, 0x85, 0x43, 0x00, 0x90}, 7},
    {0x584770, 0x584A79, SetPlayerColor, {0xFF, 0x25, 0x79, 0x4A, 0x58, 0x00, 0x90}, 7},
    {0x53C900, 0x53C8F1, ShutdownGame, {0xFF, 0x25, 0xF1, 0xC8, 0x53, 0x00, 0x90}, 7},
    {0x4B5AC0, 0x4B5ABC, HandleDamage, {0xFF, 0x25, 0xBC, 0x5A, 0x4B, 0x00}, 6},
    {0x738F3A, 0x738B1B, FilterVehicleSync, {0xFF, 0x25, 0x1B, 0x8B, 0x73, 0x00}, 6},
    {0x738877, 0x73885B, FilterVehicleSync, {0xFF, 0x25, 0x5B, 0x88, 0x73, 0x00}, 6},
    {0x6A0050, 0x6A0043, ProcessNetStats, {0xFF, 0x25, 0x43, 0x00, 0x6A, 0x00, 0x90, 0x90, 0x90}, 9},
    {0x7FB020, 0x59C721, CheckProcess, {0xFF, 0x25, 0x21, 0xC7, 0x59, 0x00}, 6},
    {0x538090, 0x538084, ProcessWeaponPickup, {0xFF, 0x25, 0x84, 0x80, 0x53, 0x00, 0x90}, 7},
    {0x5534B0, 0x5534A6, ProcessObjectUpdate, {0xFF, 0x25, 0xA6, 0x34, 0x55, 0x00, 0x90, 0x90, 0x90}, 9},
    {0x4B35A0, 0x4B3433, GuardLocalPlayerAction, {0xFF, 0x25, 0x33, 0x34, 0x4B, 0x00}, 6},
};

struct RelativeCall {
  uintptr_t site;
  Handler handler;
};

const RelativeCall g_relative_calls[] = {
    {0x7330A2, ApplyWeaponPreset},   {0x5689FD, ProcessPlayerSyncAll}, {0x53EA03, ProcessGameLogic},
    {0x501B1D, ProcessPlayerUpdate}, {0x501B42, ProcessPlayerUpdate},  {0x501FC2, ProcessPlayerUpdate},
    {0x502067, ProcessPlayerUpdate}, {0x5021AE, ProcessPlayerUpdate},  {0x5869BF, CreatePlayerObject},
    {0x5759E4, CreatePlayerObject},  {0x609A4E, SetPlayerSkin},        {0x6F8CF8, ResetHealth},
    {0x4E7427, GetNetworkData},      {0x6FDED6, InitProcess},          {0x6D0E7E, SetProcessParam},
    {0x586C0A, GetPlayerPointer},    {0x718599, ProcessCameraShake},   {0x5648D3, HookObjectUpdate},
    {0x53DFDD, ProcessObjectCreate}, {0x53E019, ProcessObjectDelete},  {0x5342F9, ProcessMission},
    {0x41B02E, ProcessLineOfSight},  {0x41AF80, CheckLineOfSight},     {0x41AB78, IsVehicleValid},
    {0x6E0954, CheckVehicleSync},    {0x6B2BCB, CheckVehicleSync},     {0x4F77DA, CheckVehicleSync},
};

void *At(uintptr_t address) {
  return reinterpret_cast<void *>(address);
}

void RedirectPointer(uintptr_t slot, Handler handler) {
  WriteDword(At(slot), static_cast<unsigned>(reinterpret_cast<uintptr_t>(handler)));
}

void InstallCodeHook(const CodeHook &hook) {
  WriteDword(At(hook.slot), static_cast<unsigned>(reinterpret_cast<uintptr_t>(hook.handler)));
  WriteBytes(At(hook.target), hook.trampoline, hook.size);
}

void WriteRelativeCall(uintptr_t address, void (*target)()) {
  uintptr_t next = address + 5;
  WriteByte(At(address), 0xE8);
  WriteDword(At(address + 1),
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(target) - next));
}

}  // namespace

void PatchMainHooks() {
  WriteDword(At(0x53EB13),
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_frame_callback) - 0x53EB17));
  WriteRelativeCall(0x53E981, MainTick);
}

void PatchAllHooks() {
  PatchMainHooks();
  samp::util::WriteLogLine("hooks installed");
}

}  // namespace samp::game
