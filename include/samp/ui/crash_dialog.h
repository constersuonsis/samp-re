#pragma once

#include <windows.h>

namespace samp::ui {

void SaveCrashContext(CONTEXT *context);
void BuildCrashReport(bool extended);
void DumpStackHex(unsigned address, int size, char *target, bool as_dwords);
int SendCrashReport();
INT_PTR CALLBACK CrashReportDialog(HWND dialog, UINT message, WPARAM param, LPARAM reserved);
int ShowCrashDialog(HINSTANCE module);

}  // namespace samp::ui
