#include "samp/ui/scoreboard.h"

#include <windows.h>

#include <cstddef>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

void WriteFloatBits(unsigned char *base, size_t offset, unsigned bits) {
  float value = 0.0f;
  std::memcpy(&value, &bits, sizeof(value));
  std::memcpy(base + offset, &value, sizeof(value));
}

float ReadFloat(const unsigned char *base, size_t offset) {
  float value = 0.0f;
  std::memcpy(&value, base + offset, sizeof(value));
  return value;
}

int ScreenWidth() {
  return *reinterpret_cast<volatile int *>(0xC17044);
}

unsigned GameTime() {
  static LARGE_INTEGER frequency = {};
  if (!frequency.QuadPart && !::QueryPerformanceFrequency(&frequency)) {
    return 0;
  }
  LARGE_INTEGER counter = {};
  if (!::QueryPerformanceCounter(&counter)) {
    return 0;
  }
  return static_cast<unsigned>((1000LL * counter.QuadPart) / frequency.QuadPart);
}

}  // namespace

Scoreboard *CreateScoreboard(unsigned device) {
  Scoreboard *board = new Scoreboard();
  WriteDword(board->raw, 48, device);
  WriteDword(board->raw, 52, 0);
  WriteDword(board->raw, 56, 0);
  WriteDword(board->raw, 60, 0);
  WriteDword(board->raw, 64, 0);
  WriteDword(board->raw, 4, 0);
  if (device && ScreenWidth() > 800) {
    WriteFloatBits(board->raw, 20, 1145569280);
    WriteFloatBits(board->raw, 24, 1142292480);
  } else {
    WriteFloatBits(board->raw, 20, 1142947840);
    WriteFloatBits(board->raw, 24, 1139802112);
  }
  WriteFloatBits(board->raw, 28, 1114636288);
  WriteDword(board->raw, 32, 0);
  WriteFloatBits(board->raw, 36, 1034818683);
  WriteFloatBits(board->raw, 40, 1049100288);
  WriteFloatBits(board->raw, 44, 1054867456);
  CenterScoreboard(board);
  WriteDword(board->raw, 0, 0);
  return board;
}

void CenterScoreboard(Scoreboard *board) {
  HWND window = *reinterpret_cast<HWND *>(0xC97C1C);
  RECT area = {};
  ::GetClientRect(window, &area);
  WriteFloatBits(board->raw, 16, 1065353216);
  float width = ReadFloat(board->raw, 20);
  float height = ReadFloat(board->raw, 24);
  float x = static_cast<float>(area.right) * 0.5f - width * 0.5f;
  float y = static_cast<float>(area.bottom) * 0.5f - height * 0.5f;
  std::memcpy(board->raw + 8, &x, sizeof(x));
  std::memcpy(board->raw + 12, &y, sizeof(y));
}

ProgressScreen *CreateProgressScreen(unsigned device) {
  ProgressScreen *screen = new ProgressScreen();
  WriteDword(screen->raw, 0, 0);
  WriteDword(screen->raw, 4, 0);
  WriteDword(screen->raw, 8, device);
  WriteDword(screen->raw, 40, 0);
  WriteDword(screen->raw, 48, 0);
  WriteDword(screen->raw, 20, 640);
  WriteDword(screen->raw, 24, 300);
  WriteDword(screen->raw, 28, 210);
  WriteDword(screen->raw, 32, 30);
  WriteDword(screen->raw, 52, 3);
  std::memcpy(screen->raw + 56, "Type", sizeof("Type"));
  std::memcpy(screen->raw + 185, "ID", sizeof("ID"));
  std::memcpy(screen->raw + 314, "Progress", sizeof("Progress"));
  WriteDword(screen->raw, 572, 180);
  WriteDword(screen->raw, 576, 220);
  WriteDword(screen->raw, 588, 0);
  WriteDword(screen->raw, 592, 0xFFFFFFFF);
  WriteDword(screen->raw, 604, 0);
  WriteDword(screen->raw, 600, 0);
  WriteDword(screen->raw, 596, GameTime());
  return screen;
}

}  // namespace samp::ui
