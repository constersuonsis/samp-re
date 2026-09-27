#pragma once

namespace samp::ui {

class RenderDevice;
struct Font;

struct ChatWindow {
  unsigned char raw[0x63EA] = {};
};

struct SecondChat {
  unsigned char raw[0x1AFC] = {};
};

ChatWindow *CreateChatWindow(RenderDevice *device, Font *font, const char *log_path);
SecondChat *CreateSecondChat(void *device);
void SetChatPageSize(ChatWindow *chat, int size);
void SetChatTimestamp(ChatWindow *chat, bool enabled);
void AddChatMessage(ChatWindow *chat, const char *text);
void RegisterCommand(ChatWindow *chat, const char *name, void (*handler)());
void RegisterSampCommands(ChatWindow *chat);

}  // namespace samp::ui
