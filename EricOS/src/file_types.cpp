#include <Arduino.h>
#include <string.h>
#include "file_types.h"

namespace {
bool hasExtension(const char *name, const char *extension) {
  if (name == nullptr || extension == nullptr) return false;
  const size_t nameLength = strlen(name);
  const size_t extensionLength = strlen(extension);
  return nameLength >= extensionLength && strcmp(name + nameLength - extensionLength, extension) == 0;
}
}

namespace EricFileTypes {
bool isEditableText(const char *name) {
  static const char *const extensions[] = {
    ".TXT", ".txt", ".MD", ".md", ".LOG", ".log", ".CSV", ".csv",
    ".INI", ".ini", ".CFG", ".cfg", ".JSON", ".json", ".BAS", ".bas",
    ".C", ".c", ".H", ".h", ".CPP", ".cpp", ".PY", ".py"
  };
  for (const char *extension : extensions) if (hasExtension(name, extension)) return true;
  return false;
}

bool isWav(const char *name) { return hasExtension(name, ".WAV") || hasExtension(name, ".wav"); }
bool isBmp(const char *name) { return hasExtension(name, ".BMP") || hasExtension(name, ".bmp"); }
bool isJpeg(const char *name) { return hasExtension(name, ".JPG") || hasExtension(name, ".jpg") || hasExtension(name, ".JPEG") || hasExtension(name, ".jpeg"); }
bool isMp3(const char *name) { return hasExtension(name, ".MP3") || hasExtension(name, ".mp3"); }
bool isAudio(const char *name) { return isMp3(name); }
bool isGameBoyRom(const char *name) { return hasExtension(name, ".GB") || hasExtension(name, ".gb"); }

const char *openHint(const char *name) {
  if (isEditableText(name)) return "Text/source file";
  if (isAudio(name)) return "MP3 audio file";
  if (isBmp(name)) return "BMP image file";
  if (isJpeg(name)) return "JPEG image file";
  return "Unsupported file";
}
}
