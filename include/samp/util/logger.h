#pragma once

namespace samp::util {

void InitLogger(const char *directory);
void WriteLogLine(const char *stage);
void WriteLogValue(const char *name, const char *value);
void WriteLogNumber(const char *name, unsigned value);
void WriteLogException(unsigned code, unsigned address);

}  // namespace samp::util
