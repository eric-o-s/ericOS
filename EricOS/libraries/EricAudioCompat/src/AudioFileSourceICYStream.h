#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include "AudioFileSource.h"





class AudioFileSourceICYStream : public AudioFileSource {
public:
  explicit AudioFileSourceICYStream(const char *url, uint32_t connectTimeoutMs = 6000);
  void close() override;
  bool isOpen() override;
  int available() override;
  size_t readBytes(uint8_t *data, size_t length) override;
  bool isEndOfStream() override { return _eof; }
  
  
  int httpStatus() const { return _httpStatus; }
  
  const char *httpReason() const { return _httpReason.c_str(); }

private:
  bool connect();
  bool sendRequest();
  bool readStatusLine();
  void readHeaders();
  
  void serviceMetadata();
  
  
  bool parseUrl();

  WiFiClient _client;
  String _url;
  String _host;
  String _path;
  uint16_t _port = 80;
  bool _open = false;
  bool _eof = false;
  int _metaInterval = 0;
  int _metaLeft = 0;
  int _httpStatus = 0;
  String _httpReason = "";
};
