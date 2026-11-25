#pragma once



namespace EricAudio {
bool begin();
bool play(const char *path);
void pump();
void stop();
bool isPlaying();
const char *path();
const char *message();
}
