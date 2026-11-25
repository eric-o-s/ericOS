#include "AudioFileSourceSD.h"
#include <SD.h>

bool AudioFileSourceSD::open(const char *filename) {
  close();
  if (filename == nullptr || filename[0] == '\0') return false;
  _file = SD.open(filename, FILE_READ);
  if (!_file) return false;
  _open = true;
  _eof = false;
  return true;
}

void AudioFileSourceSD::close() {
  if (_file) _file.close();
  _open = false;
  _eof = false;
}

bool AudioFileSourceSD::isOpen() { return _open && bool(_file); }

int AudioFileSourceSD::available() {
  if (!isOpen()) return 0;
  return _file.available();
}

size_t AudioFileSourceSD::readBytes(uint8_t *data, size_t length) {
  if (!isOpen() || data == nullptr || length == 0) return 0;
  const size_t read = _file.read(data, length);
  if (read == 0) _eof = true;
  return read;
}
