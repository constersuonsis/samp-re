#pragma once

namespace samp::audio {

bool InitAudioStreams();
void SetAudioVolume(float volume);
void StopAudioStreams();
void PlaySoundByID(int id);
unsigned char *AudioFlag();

}  // namespace samp::audio
