#pragma once

namespace samp::game {

void RenderFrame(void *scene);
unsigned long long RenderFrame2();
void FixWidescreen();
void SetWidescreenFix(bool enabled);
void ArmFovRestore(unsigned view_a, unsigned view_b);
void RequestQuit();
void NoteRenderCheck();

}  // namespace samp::game
