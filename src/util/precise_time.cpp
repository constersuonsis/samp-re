#include "samp/util/precise_time.h"

#include <windows.h>

namespace samp::util {
namespace {

bool g_frequency_ready = false;
bool g_frequency_ok = false;
long long g_frequency = 0;
bool g_counter_started = false;
long long g_last_counter = 0;
bool g_fallback_started = false;
double g_last_fallback = 0.0;

}  // namespace

double PreciseDeltaTime() {
  if (!g_frequency_ready) {
    g_frequency_ready = true;
    LARGE_INTEGER frequency = {};
    g_frequency_ok = ::QueryPerformanceFrequency(&frequency) != 0;
    if (g_frequency_ok) {
      g_frequency = frequency.QuadPart;
    }
  }
  if (g_frequency_ok) {
    LARGE_INTEGER now = {};
    ::QueryPerformanceCounter(&now);
    long long previous = 0;
    if (g_counter_started) {
      previous = g_last_counter;
    } else {
      g_counter_started = true;
      previous = now.QuadPart;
    }
    g_last_counter = now.QuadPart;
    return static_cast<double>(now.QuadPart - previous) / static_cast<double>(g_frequency);
  }
  double now_seconds = static_cast<double>(::timeGetTime()) * 0.001;
  double delta = 0.0;
  if (g_fallback_started) {
    delta = now_seconds - g_last_fallback;
  } else {
    g_fallback_started = true;
  }
  g_last_fallback = now_seconds;
  return delta;
}

}  // namespace samp::util
