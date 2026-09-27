#include "samp/game/vehicle_preview.h"

namespace samp::game {
namespace {

using EmptyCall = void(__cdecl *)();
using CreateCall = void(__cdecl *)(float, float, const char *);

void CallGame(unsigned address) {
  reinterpret_cast<EmptyCall>(address)();
}

}  // namespace

void PreviewGetRotation() {
  CallGame(0x719380);
}

void PreviewDelete() {
  CallGame(0x719430);
}

void PreviewSetRotation() {
  CallGame(0x719600);
}

void PreviewSetSpeed() {
  CallGame(0x719610);
}

void PreviewGetModel() {
  CallGame(0x7194D0);
}

void PreviewSetDoor() {
  CallGame(0x7194E0);
}

void PreviewGetHealth() {
  CallGame(0x7195C0);
}

void PreviewSetColor() {
  CallGame(0x7195E0);
}

void PreviewSetHealth() {
  CallGame(0x7195B0);
}

void PreviewSetPosition() {
  CallGame(0x719510);
}

void PreviewGetPosition() {
  CallGame(0x719590);
}

void PreviewSetWindow() {
  CallGame(0x719570);
}

void PreviewGetSpeed() {
  CallGame(0x719490);
}

void PreviewGetMaxCount() {
  CallGame(0x69DE90);
}

void PreviewDestroyAll() {
  CallGame(0x69E160);
}

void PreviewCreate(float x, float y, const char *model) {
  reinterpret_cast<CreateCall>(0x71A700)(x, y, model);
}

}  // namespace samp::game
