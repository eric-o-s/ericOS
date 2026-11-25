#pragma once



namespace EricFileTypes {
bool isEditableText(const char *name);
bool isAudio(const char *name);
bool isWav(const char *name);
bool isBmp(const char *name);
bool isJpeg(const char *name);

bool isGameBoyRom(const char *name);
const char *openHint(const char *name);
}
