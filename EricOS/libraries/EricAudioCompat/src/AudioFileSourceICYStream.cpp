#include "AudioFileSourceICYStream.h"
#include <WiFi.h>

namespace {



constexpr char RequestTemplate[] =
    "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: E.R.I.C-Pico/1.0\r\n"
    "Icy-MetaData: 1\r\n"
    "Connection: close\r\n"
    "\r\n";

uint32_t waitedMs(uint32_t started) {
  return millis() - started;
}
} 

AudioFileSourceICYStream::AudioFileSourceICYStream(const char *url, uint32_t connectTimeoutMs)
  : _url(url == nullptr ? "" : url) {
  if (_url.length() == 0) return;
  if (!parseUrl()) return;
  connectTimeoutMs = connectTimeoutMs == 0 ? 1 : connectTimeoutMs;
  const uint32_t started = millis();
  while (waitedMs(started) < connectTimeoutMs) {
    if (connect()) return;
    delay(50);
  }
}

bool AudioFileSourceICYStream::parseUrl() {
  String rest = _url;
  
  
  if (rest.startsWith("http://")) rest = rest.substring(7);
  else if (rest.startsWith("https://")) {
    _httpStatus = 0;
    _httpReason = "HTTPS UNSUPPORTED";
    return false;
  } else if (rest.startsWith("icecast://")) {
    rest = rest.substring(10);
  } else {
    _httpReason = "UNSUPPORTED URL";
    return false;
  }

  const int pathAt = rest.indexOf('/');
  if (pathAt < 0) {
    _host = rest;
    _path = "/";
  } else {
    _host = rest.substring(0, pathAt);
    _path = rest.substring(pathAt);
  }
  const int colon = _host.indexOf(':');
  if (colon >= 0) {
    const long value = _host.substring(colon + 1).toInt();
    if (value <= 0 || value > 65535) {
      _httpReason = "BAD PORT";
      return false;
    }
    _port = static_cast<uint16_t>(value);
    _host = _host.substring(0, colon);
  }
  
  const int at = _host.indexOf('@');
  if (at >= 0) _host = _host.substring(at + 1);
  return _host.length() > 0;
}

bool AudioFileSourceICYStream::connect() {
  _client.stop();
  _httpStatus = 0;
  _httpReason = "";
  _eof = false;
  _metaInterval = 0;
  _metaLeft = 0;
  if (!_client.connect(_host, _port)) {
    _httpReason = "NO CONNECTION";
    return false;
  }
  if (!sendRequest()) {
    _client.stop();
    return false;
  }
  if (!readStatusLine()) {
    _client.stop();
    return false;
  }
  readHeaders();
  _open = _httpStatus == 200;
  if (!_open) {
    _client.stop();
    return false;
  }
  if (_metaInterval > 0) _metaLeft = _metaInterval;
  return true;
}

bool AudioFileSourceICYStream::sendRequest() {
  char request[256];
  const int written = snprintf(request, sizeof(request), RequestTemplate, _path.c_str(), _host.c_str());
  if (written <= 0 || static_cast<size_t>(written) >= sizeof(request)) return false;
  
  
  _client.setNoDelay(true);
  return _client.write(reinterpret_cast<const uint8_t *>(request), static_cast<size_t>(written)) == written;
}

bool AudioFileSourceICYStream::readStatusLine() {
  const uint32_t started = millis();
  while (!_client.available()) {
    if (!_client.connected()) {
      _httpReason = "NO RESPONSE";
      return false;
    }
    if (waitedMs(started) > 6000UL) {
      _httpReason = "NO RESPONSE";
      return false;
    }
    delay(2);
  }
  const String status = _client.readStringUntil('\n');
  if (!status.startsWith("HTTP/1.")) {
    _httpReason = "NOT HTTP";
    return false;
  }
  const int firstSpace = status.indexOf(' ');
  if (firstSpace < 0) {
    _httpReason = "BAD STATUS";
    return false;
  }
  String rest = status.substring(firstSpace + 1);
  rest.trim();
  const int code = rest.toInt();
  if (code < 100 || code > 599) {
    _httpReason = "BAD STATUS";
    return false;
  }
  _httpStatus = code;
  const int secondSpace = rest.indexOf(' ');
  _httpReason = secondSpace >= 0 ? rest.substring(secondSpace + 1) : String("");
  _httpReason.trim();
  return true;
}

void AudioFileSourceICYStream::readHeaders() {
  
  
  const uint32_t started = millis();
  while (waitedMs(started) < 6000UL) {
    if (!_client.connected()) return;
    if (!_client.available()) {
      delay(2);
      continue;
    }
    String line = _client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return; 
    
    const String lower = line.substring(0, 3);
    if (lower.equalsIgnoreCase("icy")) {
      const int colon = line.indexOf(':');
      if (colon > 0) {
        const String name = line.substring(0, colon);
        const String value = line.substring(colon + 1);
        if (name.equalsIgnoreCase("icy-metaint")) _metaInterval = value.toInt();
      }
    }
  }
}

void AudioFileSourceICYStream::serviceMetadata() {
  if (_metaInterval <= 0 || _metaLeft > 0) return;
  
  
  const uint32_t started = millis();
  while (!_client.available()) {
    if (!_client.connected() || waitedMs(started) > 2000UL) return;
    delay(1);
  }
  const int lengthByte = _client.read();
  int remaining = lengthByte * 16;
  while (remaining > 0 && _client.connected()) {
    while (!_client.available() && _client.connected()) delay(1);
    if (!_client.connected()) return;
    const int value = _client.read();
    if (value < 0) return;
    remaining--;
  }
  _metaLeft = _metaInterval;
}

void AudioFileSourceICYStream::close() {
  _client.stop();
  _open = false;
  _metaInterval = 0;
  _metaLeft = 0;
}

bool AudioFileSourceICYStream::isOpen() { return _open && _client.connected(); }

int AudioFileSourceICYStream::available() {
  if (!isOpen()) return 0;
  return _client.available();
}

size_t AudioFileSourceICYStream::readBytes(uint8_t *data, size_t length) {
  if (!isOpen() || data == nullptr || length == 0) return 0;
  serviceMetadata();
  if (_metaLeft > 0) {
    const size_t allowed = (size_t)_metaLeft < length ? (size_t)_metaLeft : length;
    const size_t read = (size_t)_client.readBytes(data, allowed);
    _metaLeft -= (int)read;
    if (read == 0 && !_client.connected()) _eof = true;
    return read;
  }
  const size_t read = (size_t)_client.readBytes(data, length);
  if (read == 0) _eof = !_client.connected();
  return read;
}
