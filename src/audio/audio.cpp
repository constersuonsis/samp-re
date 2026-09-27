#include "samp/audio/audio.h"

#include <windows.h>

namespace samp::audio {
namespace {

struct BassApi {
  int(__stdcall *Free)() = nullptr;
  int(__stdcall *Init)(int, unsigned, unsigned, void *, void *) = nullptr;
  int(__stdcall *SetConfig)(unsigned, unsigned long long) = nullptr;
  int(__stdcall *SetConfigPtr)(unsigned, const void *) = nullptr;
  int(__stdcall *SetEAXParameters)(int, float, float, float) = nullptr;
};

BassApi g_bass;
bool g_audio_ready = false;
unsigned char g_audio_flag = 0;
unsigned char g_stream_active = 0;

bool LoadBass(BassApi &api) {
  HMODULE module = ::LoadLibraryA("bass.dll");
  if (!module) {
    return false;
  }
  api.Free = reinterpret_cast<decltype(api.Free)>(::GetProcAddress(module, "BASS_Free"));
  api.Init = reinterpret_cast<decltype(api.Init)>(::GetProcAddress(module, "BASS_Init"));
  api.SetConfig =
      reinterpret_cast<decltype(api.SetConfig)>(::GetProcAddress(module, "BASS_SetConfig"));
  api.SetConfigPtr = reinterpret_cast<decltype(api.SetConfigPtr)>(
      ::GetProcAddress(module, "BASS_SetConfigPtr"));
  api.SetEAXParameters = reinterpret_cast<decltype(api.SetEAXParameters)>(
      ::GetProcAddress(module, "BASS_SetEAXParameters"));
  return api.Free && api.Init && api.SetConfig && api.SetConfigPtr && api.SetEAXParameters;
}

float AudioVolume() {
  return 1.0f;
}

}  // namespace

bool InitAudioStreams() {
  if (!LoadBass(g_bass)) {
    return false;
  }
  g_bass.Free();
  if (!g_bass.Init(-1, 44100, 0, nullptr, nullptr)) {
    return false;
  }
  g_bass.SetConfigPtr(16, "SA-MP/0.3");
  g_bass.SetConfig(5, static_cast<unsigned long long>(AudioVolume() * 7000.0));
  g_bass.SetConfig(21, 1);
  g_bass.SetConfig(11, 10000);
  g_bass.SetEAXParameters(-1, 0.0f, -1.0f, -1.0f);
  g_audio_ready = true;
  g_audio_flag = 1;
  return true;
}

void SetAudioVolume(float volume) {
  if (!g_audio_ready) {
    return;
  }
  g_bass.SetConfig(5, static_cast<unsigned long long>(volume * 7000.0));
}

void PlaySoundByID(int id) {  using GamePlaySound = int(__cdecl *)(int, int, int);
  reinterpret_cast<GamePlaySound>(0x507DC0)(11975824, id, 0);
}

void StopAudioStreams() {
  if (!g_audio_flag) {
    return;
  }
  if (!g_stream_active) {
    return;
  }
  PlaySoundByID(-1);
  using GameStopAll = int(__cdecl *)(int, int, int);
  reinterpret_cast<GameStopAll>(0x506F70)(11975824, 0, 0);
}

unsigned char *AudioFlag() {
  return &g_audio_flag;
}

}  // namespace samp::audio
