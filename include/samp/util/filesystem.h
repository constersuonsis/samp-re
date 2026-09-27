#pragma once

namespace samp::util {

bool FileExists(const char *path);
bool EnsureSampDirectories(char *buffer, size_t capacity);

}  // namespace samp::util
