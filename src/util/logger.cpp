#include "samp/util/logger.h"

#include <windows.h>

#include <cstddef>
#include <cstdio>

namespace samp::util {
namespace {

char g_log_path[MAX_PATH] = {};

void AppendRaw(const char *text, size_t length) {
  if (!g_log_path[0]) {
    return;
  }
  std::FILE *file = nullptr;
  if (fopen_s(&file, g_log_path, "a") != 0 || !file) {
    return;
  }
  std::fwrite(text, 1, length, file);
  std::fclose(file);
}

size_t FormatUnsigned(char *out, size_t capacity, unsigned value, unsigned base) {
  char digits[32] = {};
  size_t count = 0;
  if (!value) {
    digits[count++] = '0';
  }
  while (value && count < sizeof(digits)) {
    unsigned digit = value % base;
    digits[count++] = static_cast<char>(digit < 10 ? '0' + digit : 'A' + digit - 10);
    value /= base;
  }
  size_t length = 0;
  while (count > 0 && length + 1 < capacity) {
    out[length++] = digits[--count];
  }
  out[length] = 0;
  return length;
}

}  // namespace

void InitLogger(const char *directory) {
  if (!directory) {
    return;
  }
  size_t length = 0;
  while (directory[length] != 0 && length + 1 < sizeof(g_log_path)) {
    g_log_path[length] = directory[length];
    ++length;
  }
  const char *tail = "\\samp_debug.log";
  size_t tail_length = 0;
  while (tail[tail_length] != 0 && length + 1 < sizeof(g_log_path)) {
    g_log_path[length++] = tail[tail_length++];
  }
  g_log_path[length] = 0;
  std::FILE *file = nullptr;
  if (fopen_s(&file, g_log_path, "w") == 0 && file) {
    const char *header = "samp log start\r\n";
    size_t header_length = 0;
    while (header[header_length] != 0) {
      ++header_length;
    }
    std::fwrite(header, 1, header_length, file);
    std::fclose(file);
  }
}

void WriteLogLine(const char *stage) {  char line[512] = {};
  size_t length = 0;
  char number[32] = {};
  FormatUnsigned(number, sizeof(number), ::GetTickCount(), 10);
  size_t i = 0;
  while (number[i] != 0 && length + 1 < sizeof(line)) {
    line[length++] = number[i++];
  }
  line[length++] = ' ';
  i = 0;
  while (stage[i] != 0 && length + 1 < sizeof(line)) {
    line[length++] = stage[i++];
  }
  line[length++] = '\r';
  line[length++] = '\n';
  AppendRaw(line, length);
}

void WriteLogValue(const char *name, const char *value) {
  char line[512] = {};
  size_t length = 0;
  char number[32] = {};
  FormatUnsigned(number, sizeof(number), ::GetTickCount(), 10);
  size_t i = 0;
  while (number[i] != 0 && length + 1 < sizeof(line)) {
    line[length++] = number[i++];
  }
  line[length++] = ' ';
  while (*name && length + 1 < sizeof(line)) {
    line[length++] = *name++;
  }
  line[length++] = '=';
  while (*value && length + 1 < sizeof(line)) {
    line[length++] = *value++;
  }
  line[length++] = '\r';
  line[length++] = '\n';
  AppendRaw(line, length);
}

void WriteLogNumber(const char *name, unsigned value) {
  char digits[32] = {};
  FormatUnsigned(digits, sizeof(digits), value, 10);
  WriteLogValue(name, digits);
}

void WriteLogException(unsigned code, unsigned address) {
  char line[128] = {};
  size_t length = 0;
  const char *prefix = "exception code=";
  while (*prefix && length + 1 < sizeof(line)) {
    line[length++] = *prefix++;
  }
  char number[32] = {};
  FormatUnsigned(number, sizeof(number), code, 16);
  size_t i = 0;
  while (number[i] != 0 && length + 1 < sizeof(line)) {
    line[length++] = number[i++];
  }
  const char *middle = " address=";
  while (*middle && length + 1 < sizeof(line)) {
    line[length++] = *middle++;
  }
  FormatUnsigned(number, sizeof(number), address, 16);
  i = 0;
  while (number[i] != 0 && length + 1 < sizeof(line)) {
    line[length++] = number[i++];
  }
  line[length++] = '\r';
  line[length++] = '\n';
  AppendRaw(line, length);
}

}  // namespace samp::util
