#include "samp/ui/chat.h"
#include "samp/ui/font.h"
#include "samp/ui/render_device.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <cstring>

namespace samp::ui {
namespace {

void WriteDword(unsigned char *base, size_t offset, unsigned value) {
  std::memcpy(base + offset, &value, sizeof(value));
}

unsigned ReadDword(const unsigned char *base, size_t offset) {
  unsigned value = 0;
  std::memcpy(&value, base + offset, sizeof(value));
  return value;
}

void ReleaseSlot(RenderDevice *device, unsigned char *base, size_t offset) {
  unsigned slot = ReadDword(base, offset);
  if (slot) {
    device->ReleaseSurface(reinterpret_cast<void *>(slot));
    WriteDword(base, offset, 0);
  }
}

void MeasureDirectMode(RenderDevice *device, ChatWindow *chat) {
  TextBounds bounds;
  device->MeasureText(nullptr, "Y", bounds);
  WriteDword(chat->raw, 25570, static_cast<unsigned>(bounds.bottom - bounds.top));
  device->MeasureText(nullptr, "[19:58:34]", bounds);
  WriteDword(chat->raw, 25574, static_cast<unsigned>(bounds.right - bounds.left));
}

void InitializeRendering(RenderDevice *device, ChatWindow *chat) {
  WriteDword(chat->raw, 25522, 1);
  ReleaseSlot(device, chat->raw, 25534);
  ReleaseSlot(device, chat->raw, 25530);
  ReleaseSlot(device, chat->raw, 25526);
  DisplayMode mode;
  if (!device->DisplayModeInfo(mode)) {
    WriteDword(chat->raw, 25522, 0);
    return;
  }
  std::memcpy(chat->raw + 25538, &mode, sizeof(mode));
  void *texture = nullptr;
  if (mode.width > 0x400) {
    texture = device->CreateTexture(2048, 1024);
  } else {
    texture = device->CreateTexture(1024, 512);
  }
  if (!texture) {
    AddChatMessage(chat, "ChatWindow: Can't create a render surface texture. Will use direct mode.");
    WriteDword(chat->raw, 25522, 0);
    return;
  }
  void *level = device->SurfaceLevel(texture);
  WriteDword(chat->raw, 25530, static_cast<unsigned>(reinterpret_cast<uintptr_t>(texture)));
  WriteDword(chat->raw, 25534, static_cast<unsigned>(reinterpret_cast<uintptr_t>(level)));
  SurfaceDesc texture_desc;
  if (!level || !device->SurfaceDescription(level, texture_desc)) {
    AddChatMessage(chat, "ChatWindow: Can't describe the render surface. Will use direct mode.");
    MeasureDirectMode(device, chat);
    WriteDword(chat->raw, 25522, 0);
    return;
  }
  void *target = device->CreateRenderTarget(level, texture_desc);
  if (!target) {
    AddChatMessage(chat, "ChatWindow: Can't create a render to surface. Will use direct mode.");
    MeasureDirectMode(device, chat);
    WriteDword(chat->raw, 25522, 0);
    return;
  }
  WriteDword(chat->raw, 25526, static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)));
  MeasureDirectMode(device, chat);
  WriteDword(chat->raw, 25558, 0);
  WriteDword(chat->raw, 25562, 1);
}

}  // namespace

void AddChatMessage(ChatWindow *chat, const char *text) {
  (void)chat;
  (void)text;
}

void SetChatPageSize(ChatWindow *chat, int size) {
  if (!chat || size < 10 || size > 100) {
    return;
  }
  WriteDword(chat->raw, 0, static_cast<unsigned>(size));
  WriteDword(chat->raw, 25562, 1);
}

void SetChatTimestamp(ChatWindow *chat, bool enabled) {
  if (!chat) {
    return;
  }
  chat->raw[12] = enabled ? 1 : 0;
  WriteDword(chat->raw, 25562, 1);
}

ChatWindow *CreateChatWindow(RenderDevice *device, Font *font, const char *log_path) {
  if (!device) {
    device = &NullRenderDevice();
  }
  ChatWindow *chat = new ChatWindow();
  WriteDword(chat->raw, 25518, static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
  WriteDword(chat->raw, 25506, static_cast<unsigned>(reinterpret_cast<uintptr_t>(font)));
  WriteDword(chat->raw, 8, 2);
  WriteDword(chat->raw, 25510,
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(device->CreateSprite())));
  WriteDword(chat->raw, 25514,
             static_cast<unsigned>(reinterpret_cast<uintptr_t>(device->CreateSprite())));
  std::memset(chat->raw + 306, 0, 0x6270);
  WriteDword(chat->raw, 290, 0xFFFFFFFF);
  WriteDword(chat->raw, 294, static_cast<unsigned>(-7820702));
  WriteDword(chat->raw, 298, static_cast<unsigned>(-5651228));
  WriteDword(chat->raw, 0, 10);
  chat->raw[12] = 0;
  if (log_path && log_path[0]) {
    std::memset(chat->raw + 17, 0, 0x104);
    chat->raw[277] = 0;
    size_t length = 0;
    while (log_path[length] != 0 && length + 1 < 0x104) {
      chat->raw[17 + length] = log_path[length];
      ++length;
    }
    chat->raw[17 + length] = 0;
    std::FILE *file = nullptr;
    if (fopen_s(&file, reinterpret_cast<const char *>(chat->raw + 17), "w") == 0 && file) {
      WriteDword(chat->raw, 13, 1);
      std::fclose(file);
    }
  }
  WriteDword(chat->raw, 25534, 0);
  WriteDword(chat->raw, 25530, 0);
  WriteDword(chat->raw, 25526, 0);
  WriteDword(chat->raw, 25554, ::GetTickCount());
  WriteDword(chat->raw, 25566, 1);
  WriteDword(chat->raw, 286, 0);
  WriteDword(chat->raw, 278, 0);
  WriteDword(chat->raw, 282, 0);
  InitializeRendering(device, chat);
  return chat;
}

SecondChat *CreateSecondChat(void *device) {
  SecondChat *chat = new SecondChat();
  WriteDword(chat->raw, 0, static_cast<unsigned>(reinterpret_cast<uintptr_t>(device)));
  WriteDword(chat->raw, 8, 0);
  WriteDword(chat->raw, 5340, 0);
  WriteDword(chat->raw, 5344, 0);
  WriteDword(chat->raw, 6900, 0);
  WriteDword(chat->raw, 6896, 0xFFFFFFFF);
  std::memset(chat->raw + 5477, 0, 0x508);
  chat->raw[6765] = 0;
  chat->raw[6766] = 0;
  std::memset(chat->raw + 5348, 0, 0x80);
  chat->raw[5476] = 0;
  std::memset(chat->raw + 6767, 0, 0x80);
  chat->raw[6895] = 0;
  return chat;
}

namespace {

struct CommandEntry {
  const char *name = nullptr;
  void (*handler)() = nullptr;
};

CommandEntry g_commands[32] = {};
int g_command_count = 0;

void DisconnectCommand() {
}

void SavePositionCommand() {
}

void SaveRawPositionCommand() {
}

void SendRconCommand() {
}

void ShowMemoryCommand() {
}

void SetFpsLimitCommand() {
}

void SetPageSizeCommand() {
}

void SetFontSizeCommand() {
}

void ToggleNametagStatusCommand() {
}

void ToggleTimestampCommand() {
}

void ToggleHeadMoveCommand() {
}

void ToggleHudScaleFixCommand() {
}

void TestMessagesCommand() {
}

void SelectVehicleCommand() {
}

void SpawnVehicleCommand() {
}

void ChangeSkinCommand() {
}

void SetWeatherCommand() {
}

void SetTimeCommand() {
}

void ShowInteriorCommand() {
}

void ToggleObjectLightCommand() {
}

void NoOpCommand() {
}

void ToggleDlCommand() {
}

void ToggleChatCommand() {
}

void ToggleAudioMessagesCommand() {
}

void ToggleLogUrlsCommand() {
}

}  // namespace

void RegisterCommand(ChatWindow *chat, const char *name, void (*handler)()) {
  (void)chat;
  if (g_command_count >= 32) {
    return;
  }
  g_commands[g_command_count].name = name;
  g_commands[g_command_count].handler = handler;
  ++g_command_count;
}

void RegisterSampCommands(ChatWindow *chat) {
  RegisterCommand(chat, "quit", DisconnectCommand);
  RegisterCommand(chat, "q", DisconnectCommand);
  RegisterCommand(chat, "save", SavePositionCommand);
  RegisterCommand(chat, "rs", SaveRawPositionCommand);
  RegisterCommand(chat, "rcon", SendRconCommand);
  RegisterCommand(chat, "mem", ShowMemoryCommand);
  RegisterCommand(chat, "fpslimit", SetFpsLimitCommand);
  RegisterCommand(chat, "pagesize", SetPageSizeCommand);
  RegisterCommand(chat, "fontsize", SetFontSizeCommand);
  RegisterCommand(chat, "nametagstatus", ToggleNametagStatusCommand);
  RegisterCommand(chat, "timestamp", ToggleTimestampCommand);
  RegisterCommand(chat, "headmove", ToggleHeadMoveCommand);
  RegisterCommand(chat, "hudscalefix", ToggleHudScaleFixCommand);
  RegisterCommand(chat, "testdw", TestMessagesCommand);
  RegisterCommand(chat, "vsel", SelectVehicleCommand);
  RegisterCommand(chat, "v", SpawnVehicleCommand);
  RegisterCommand(chat, "vehicle", SpawnVehicleCommand);
  RegisterCommand(chat, "player_skin", ChangeSkinCommand);
  RegisterCommand(chat, "set_weather", SetWeatherCommand);
  RegisterCommand(chat, "set_time", SetTimeCommand);
  RegisterCommand(chat, "interior", ShowInteriorCommand);
  RegisterCommand(chat, "togobjlight", ToggleObjectLightCommand);
  RegisterCommand(chat, "cmpstat", NoOpCommand);
  RegisterCommand(chat, "dl", ToggleDlCommand);
  RegisterCommand(chat, "ctd", ToggleChatCommand);
  RegisterCommand(chat, "audiomsg", ToggleAudioMessagesCommand);
  RegisterCommand(chat, "logurls", ToggleLogUrlsCommand);
}

}  // namespace samp::ui
