#include "samp/scripting/command_manager.h"

#include <cstddef>

namespace samp::scripting {
namespace {

void *g_command_manager = nullptr;

}  // namespace

void CreateCommandManager() {
  g_command_manager = ::operator new(0xE0);
  unsigned char *cells = static_cast<unsigned char *>(g_command_manager);
  for (size_t i = 0; i < 0xE0; ++i) {
    cells[i] = 0;
  }
}

void *CommandManager() {
  return g_command_manager;
}

}  // namespace samp::scripting
