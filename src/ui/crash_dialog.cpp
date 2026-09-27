#include "samp/ui/crash_dialog.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>
#include <cstring>

namespace samp::ui {
namespace {

const size_t kReportCapacity = 65536;

CONTEXT *g_crash_context = nullptr;
char g_crash_report[kReportCapacity] = {};
int g_script_opcode = 0;
int g_debug_flag = 0;
unsigned short g_last_object = 0;

unsigned ContextDword(size_t offset) {
  if (!g_crash_context) {
    return 0;
  }
  unsigned value = 0;
  std::memcpy(&value, reinterpret_cast<const unsigned char *>(g_crash_context) + offset,
              sizeof(value));
  return value;
}

void AppendText(char *target, const char *text) {
  size_t used = std::strlen(target);
  size_t length = std::strlen(text);
  if (used + length + 1 > kReportCapacity) {
    length = kReportCapacity - used - 1;
  }
  std::memcpy(target + used, text, length);
  target[used + length] = 0;
}

const char *GameVersionName(int index) {
  if (index == 1) {
    return "US 1.0";
  }
  if (index == 2) {
    return "EU 1.0";
  }
  return "UNKNOWN";
}

}  // namespace

void SaveCrashContext(CONTEXT *context) {
  g_crash_context = context;
}

void DumpStackHex(unsigned address, int size, char *target, bool as_dwords) {
  char line[256] = {};
  if (as_dwords) {
    for (int offset = 8; offset < size; offset += 16) {
      sprintf_s(line, "+%04X: 0x%08X   0x%08X   0x%08X   0x%08X\r\n", offset,
                *reinterpret_cast<unsigned *>(address + offset - 8),
                *reinterpret_cast<unsigned *>(address + offset - 4),
                *reinterpret_cast<unsigned *>(address + offset),
                *reinterpret_cast<unsigned *>(address + offset + 4));
      AppendText(target, line);
    }
  } else {
    for (int offset = 14; offset < size; offset += 16) {
      const unsigned char *bytes = reinterpret_cast<const unsigned char *>(address);
      sprintf_s(line,
                "+%04X: %02X %02X %02X %02X   %02X %02X %02X %02X"
                "   %02X %02X %02X %02X   %02X %02X %02X %02X\r\n",
                offset, bytes[offset - 14], bytes[offset - 13], bytes[offset - 12],
                bytes[offset - 11], bytes[offset - 10], bytes[offset - 9], bytes[offset - 8],
                bytes[offset - 7], bytes[offset - 6], bytes[offset - 5], bytes[offset - 4],
                bytes[offset - 3], bytes[offset - 2], bytes[offset - 1], bytes[offset],
                bytes[offset + 1]);
      AppendText(target, line);
    }
  }
}

void BuildCrashReport(bool extended) {
  char line[512] = {};
  sprintf_s(g_crash_report,
            "SA-MP %s\r\n"
            "Exception At Address: 0x%08X\r\n"
            "Base: 0x%08X\r\n"
            "\r\n"
            "Registers:\r\n"
            "EAX: 0x%08X\tEBX: 0x%08X\tECX: 0x%08X\tEDX: 0x%08X\r\n"
            "ESI: 0x%08X\tEDI: 0x%08X\tEBP: 0x%08X\tESP: 0x%08X\r\n"
            "EFLAGS: 0x%08X\r\n"
            "\r\n"
            "Stack:\r\n",
            "0.3.7-R3", ContextDword(184), 0x400000, ContextDword(176), ContextDword(164),
            ContextDword(172), ContextDword(168), ContextDword(160), ContextDword(156),
            ContextDword(180), ContextDword(196), ContextDword(192));
  DumpStackHex(ContextDword(196), 640, g_crash_report, true);
  sprintf_s(line, "\r\nSCM Op: 0x%X, lDbg: %d LastRendObj: %u\r\n", g_script_opcode, g_debug_flag,
            g_last_object);
  AppendText(g_crash_report, line);
  sprintf_s(line, "\r\nGame Version: %s\r\n", GameVersionName(0));
  AppendText(g_crash_report, line);
  if (extended) {
    sprintf_s(line, "\r\nModules:\r\n");
    AppendText(g_crash_report, line);
  }
}

int SendCrashReport() {
  BuildCrashReport(true);
  return 0;
}

INT_PTR CALLBACK CrashReportDialog(HWND dialog, UINT message, WPARAM param, LPARAM reserved) {
  (void)reserved;
  switch (message) {
    case WM_CLOSE: {
      ::EndDialog(dialog, 1);
      HWND game_window = *reinterpret_cast<HWND *>(0xC97C1C);
      ::ShowWindow(game_window, SW_HIDE);
      ::DestroyWindow(game_window);
      return 0;
    }
    case WM_INITDIALOG: {
      BuildCrashReport(false);
      ::SetDlgItemTextA(dialog, 1000, g_crash_report);
      ::SetForegroundWindow(::GetDlgItem(dialog, 101));
      ::SetFocus(::GetDlgItem(dialog, 1001));
      ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
      ::ShowCursor(1);
      return 0;
    }
    case WM_COMMAND: {
      if (LOWORD(param) == 1001) {
        ::EndDialog(dialog, 1);
        return 0;
      }
      if (LOWORD(param) == 1002) {
        SendCrashReport();
        ::EnableWindow(::GetDlgItem(dialog, 1002), 0);
        ::SetDlgItemTextA(dialog, 1000, "Thanks for reporting this problem.");
      }
      return 0;
    }
    default:
      return 0;
  }
}

int ShowCrashDialog(HINSTANCE module) {
  HWND game_window = *reinterpret_cast<HWND *>(0xC97C1C);
  ::ShowWindow(game_window, SW_MINIMIZE);
  return static_cast<int>(
      ::DialogBoxParamA(module, MAKEINTRESOURCEA(0x65), game_window, CrashReportDialog, 0));
}

}  // namespace samp::ui
