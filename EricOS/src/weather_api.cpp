#include "weather_api.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiNTP.h>

#include <string.h>
#include <time.h>

namespace EricWeather {
namespace {











constexpr size_t MAX_REQUEST_BYTES = 640;
constexpr size_t MAX_HEADER_BYTES = 512;




constexpr int TLS_RECV_BYTES = 4096;
constexpr int TLS_XMIT_BYTES = 512;

























constexpr time_t CLOCK_MIN_VALID_EPOCH = 1700000000;  




const IPAddress kNtpServer(162, 159, 200, 1);

bool clockIsValid() { return ::time(nullptr) >= CLOCK_MIN_VALID_EPOCH; }

enum class Stage : uint8_t { Idle, Clock, Connecting, Headers, Body, Done, Failed };




struct Request {
  Stage stage = Stage::Idle;
  const char *host = nullptr;
  char path[320] = {0};
  bool isForecast = false;
  WeatherPlace place = {};

  
  
  
  char header[MAX_HEADER_BYTES] = {0};
  size_t headerLen = 0;
  size_t headerScan = 0;
  bool headersDone = false;
  bool bodyComplete = false;
  bool statusLineSeen = false;
  int statusCode = 0;

  char body[WEATHER_BODY_CAPACITY] = {0};
  size_t bodyLen = 0;
  long contentLength = -1;

  
  
  
  WeatherPlace placeList[WEATHER_MAX_PLACES] = {};
  uint8_t placeCount = 0;
  WeatherReport report = {};

  uint32_t lastByteMs = 0;
  uint32_t clockStartMs = 0;
  char message[44] = {0};
};

Request g;




bool g_caInstalled = false;































const char kWeatherRootCa[] =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "-----END CERTIFICATE-----\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
    "CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
    "R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
    "MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
    "ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
    "EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
    "+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
    "ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
    "AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
    "zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
    "tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
    "/q4AaOeMSQ+2b1tbFfLn\n"
    "-----END CERTIFICATE-----\n";




BearSSL::WiFiClientSecure gClient;

void closeClient() {
  if (gClient) gClient.stop();
}

void fail(const char *reason) {
  g.stage = Stage::Failed;
  snprintf(g.message, sizeof(g.message), "%s", reason);
  closeClient();
}

void reset(const char *host) {
  closeClient();
  g = Request();  
  g.host = host;
  g.stage = Stage::Clock;
  snprintf(g.message, sizeof(g.message), "SETTING TIME");
}






void encodeCity(const char *src, char *dst, size_t dstSize) {
  static const char kHex[] = "0123456789ABCDEF";
  size_t out = 0;
  for (size_t i = 0; src[i] != '\0' && out + 4 < dstSize; ++i) {
    const unsigned char c = static_cast<unsigned char>(src[i]);
    const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
    if (safe) {
      dst[out++] = static_cast<char>(c);
    } else {
      dst[out++] = '%';
      dst[out++] = kHex[(c >> 4) & 0x0F];
      dst[out++] = kHex[c & 0x0F];
    }
  }
  dst[out] = '\0';
}



bool headerIs(const char *line, size_t lineLength, const char *key) {
  const size_t keyLength = strlen(key);
  if (lineLength < keyLength) return false;
  for (size_t i = 0; i < keyLength; ++i) {
    const char a = (line[i] >= 'A' && line[i] <= 'Z') ? static_cast<char>(line[i] + 32) : line[i];
    const char b = (key[i] >= 'A' && key[i] <= 'Z') ? static_cast<char>(key[i] + 32) : key[i];
    if (a != b) return false;
  }
  return true;
}




bool scanHeaders() {
  while (g.headerScan < g.headerLen) {
    
    size_t lineEnd = g.headerScan;
    while (lineEnd < g.headerLen && g.header[lineEnd] != '\n') ++lineEnd;
    if (lineEnd >= g.headerLen) return false;  

    size_t lineEndOfText = lineEnd;
    if (lineEndOfText > 0 && g.header[lineEndOfText - 1] == '\r') --lineEndOfText;

    
    
    
    
    if (lineEndOfText == g.headerScan) {
      
      
      
      
      const size_t bodyStart = lineEnd + 1;
      const size_t extra = g.headerLen > bodyStart ? g.headerLen - bodyStart : 0;
      if (extra > WEATHER_BODY_CAPACITY) {
        fail("RESPONSE TOO LARGE");
        return false;
      }
      if (extra > 0) {
        memcpy(g.body, g.header + bodyStart, extra);
        g.bodyLen = extra;
      }
      g.headersDone = true;
      g.headerLen = 0;
      return true;
    }

    if (g.headerScan == 0) {
      
      
      
      
      g.statusLineSeen = true;
      for (size_t i = 0; i + 1 < lineEndOfText; ++i) {
        const char c = g.header[i];
        if (c == ' ') {
          g.statusCode = 0;
          for (size_t d = i + 1; d < lineEndOfText && g.header[d] >= '0' && g.header[d] <= '9'; ++d) {
            g.statusCode = g.statusCode * 10 + (g.header[d] - '0');
          }
          break;
        }
      }
    }

    if (headerIs(g.header + g.headerScan, lineEndOfText - g.headerScan, "content-length")) {
      
      size_t i = g.headerScan;
      while (i < lineEndOfText && g.header[i] != ':') ++i;
      if (i < lineEndOfText) {
        long value = 0;
        for (size_t d = i + 1; d < lineEndOfText; ++d) {
          const char c = g.header[d];
          if (c >= '0' && c <= '9') value = value * 10 + (c - '0');
        }
        g.contentLength = value;
      }
    }

    g.headerScan = lineEnd + 1;
  }
  return false;
}









bool syncClock() {
  
  
  
  
  
  
  
  if (!NTP.running()) NTP.begin(kNtpServer);

  if (clockIsValid()) {
    
    
    
    
    gClient.setX509Time(::time(nullptr));
    return true;
  }

  
  
  if (g.clockStartMs == 0) {
    g.clockStartMs = millis();
  } else if (millis() - g.clockStartMs > WEATHER_CLOCK_TIMEOUT_MS) {
    fail("COULD NOT SET THE CLOCK");
    return false;
  }
  snprintf(g.message, sizeof(g.message), "SETTING TIME");
  return false;
}

void startConnect() {
  
  
  
  if (WiFi.status() != WL_CONNECTED) {
    fail("NO WIFI CONNECTION");
    return;
  }

  snprintf(g.message, sizeof(g.message), "CONTACTING");

  
  
  
  
  
  if (!g_caInstalled) {
    gClient.setCACert(kWeatherRootCa);
    g_caInstalled = true;
  }

  gClient.setBufferSizes(TLS_RECV_BYTES, TLS_XMIT_BYTES);
  
  
  
  
  
  
  
  
  gClient.setTimeout(WEATHER_CONNECT_TIMEOUT_MS);
  BearSSL::WiFiClientSecure::setTLSConnectTimeout(WEATHER_CONNECT_TIMEOUT_MS);

  
  
  
  
  if (!gClient.connect(g.host, 443)) {
    fail("COULD NOT REACH WEATHER SERVICE");
    return;
  }
  g.stage = Stage::Headers;
  g.lastByteMs = millis();
  snprintf(g.message, sizeof(g.message), "REQUESTING");
}

bool sendRequest() {
  char request[MAX_REQUEST_BYTES];
  const int n = snprintf(request, sizeof(request),
                         "GET %s HTTP/1.0\r\n"
                         "Host: %s\r\n"
                         "User-Agent: E.R.I.C-Pico/1.0\r\n"
                         "Accept: application/json\r\n"
                         "\r\n",
                         g.path, g.host);
  if (n <= 0 || static_cast<size_t>(n) >= sizeof(request)) {
    fail("REQUEST TOO LONG");
    return false;
  }
  gClient.setNoDelay(true);
  const size_t written =
      gClient.write(reinterpret_cast<const uint8_t *>(request), static_cast<size_t>(n));
  if (written != static_cast<size_t>(n)) {
    fail("SEND FAILED");
    return false;
  }
  return true;
}


void pumpRequest() {
  if (g.stage == Stage::Idle || g.stage == Stage::Done || g.stage == Stage::Failed) return;

  if (g.stage == Stage::Clock) {
    
    
    
    if (!syncClock()) return;  
    g.stage = Stage::Connecting;
  }

  if (g.stage == Stage::Connecting) {
    startConnect();
    if (g.stage == Stage::Failed) return;
    if (!sendRequest()) return;
    g.stage = Stage::Headers;
  }

  
  
  if (millis() - g.lastByteMs > WEATHER_READ_IDLE_TIMEOUT_MS) {
    fail("RESPONSE TIMED OUT");
    return;
  }

  
  
  
  if (g.bodyComplete) return;

  while (gClient.available() > 0) {
    
    
    const int inSocket = gClient.available();
    size_t room = 0;
    char *into = nullptr;
    if (!g.headersDone) {
      room = MAX_HEADER_BYTES - g.headerLen;
      into = g.header + g.headerLen;
    } else {
      room = WEATHER_BODY_CAPACITY - g.bodyLen;
      
      
      
      if (g.contentLength >= 0) {
        const size_t remaining =
            static_cast<size_t>(g.contentLength) > g.bodyLen
                ? static_cast<size_t>(g.contentLength) - g.bodyLen
                : 0;
        if (room > remaining) room = remaining;
      }
      into = g.body + g.bodyLen;
    }
    if (room == 0) {
      if (!g.headersDone) {
        
        fail("HEADERS TOO LARGE");
      } else if (g.contentLength < 0) {
        
        
        
        fail("RESPONSE TOO LARGE");
      } else {
        g.bodyComplete = true;
      }
      return;
    }

    const size_t want = static_cast<size_t>(inSocket) < room ? static_cast<size_t>(inSocket) : room;
    const int got = gClient.readBytes(into, want);
    if (got <= 0) {
      if (!gClient.connected()) break;
      continue;
    }
    g.lastByteMs = millis();

    if (!g.headersDone) {
      g.headerLen += static_cast<size_t>(got);
      if (scanHeaders()) {
        
        
        
        if (!g.statusLineSeen || g.statusCode < 200 || g.statusCode > 299) {
          fail(g.statusCode == 0 ? "NO HTTP RESPONSE" : "SERVICE REFUSED REQUEST");
          return;
        }
        g.stage = Stage::Body;
        snprintf(g.message, sizeof(g.message), "READING");
      }
    } else {
      g.bodyLen += static_cast<size_t>(got);
      if (g.contentLength >= 0 && g.bodyLen >= static_cast<size_t>(g.contentLength)) {
        g.bodyComplete = true;
        break;
      }
    }
  }

  if (g.stage != Stage::Body) return;

  
  
  
  if (g.contentLength >= 0) {
    if (g.bodyLen >= static_cast<size_t>(g.contentLength)) {
      g.bodyComplete = true;
    } else if (!gClient.connected() && gClient.available() == 0) {
      
      
      
      fail("RESPONSE TRUNCATED");
      return;
    } else {
      return;  
    }
  } else if (gClient.connected() || gClient.available() > 0) {
    return;
  }

  g.stage = Stage::Done;
  snprintf(g.message, sizeof(g.message), "DONE");
  
  
  
  closeClient();
}

}  

void pump() { pumpRequest(); }

void search(const char *city) {
  char encoded[192];
  encodeCity(city, encoded, sizeof(encoded));
  if (encoded[0] == '\0') {
    g = Request();
    g.stage = Stage::Failed;
    snprintf(g.message, sizeof(g.message), "TYPE A CITY NAME");
    return;
  }
  reset("geocoding-api.open-meteo.com");
  snprintf(g.path, sizeof(g.path),
           "/v1/search?name=%s&count=%d&language=en&format=json", encoded,
           static_cast<int>(WEATHER_MAX_PLACES));
  g.isForecast = false;
}

void fetchForecast(const WeatherPlace &place) {
  reset("api.open-meteo.com");
  snprintf(g.path, sizeof(g.path),
           "/v1/forecast"
           "?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,apparent_temperature,relative_humidity_2m,"
           "weather_code,wind_speed_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
           "precipitation_probability_max"
           "&temperature_unit=fahrenheit&wind_speed_unit=mph"
           "&timezone=auto&forecast_days=%d",
           static_cast<double>(place.latitude), static_cast<double>(place.longitude),
           static_cast<int>(WEATHER_DAYS));
  g.isForecast = true;
  g.place = place;
}




void forgetAnchors() { g_caInstalled = false; }

void abort() {
  if (g.stage == Stage::Idle || g.stage == Stage::Done || g.stage == Stage::Failed) return;
  closeClient();
  g = Request();
}

FetchState state() {
  switch (g.stage) {
    case Stage::Idle: return FetchState::Idle;
    
    
    case Stage::Clock: return FetchState::Connecting;
    case Stage::Connecting: return FetchState::Connecting;
    case Stage::Headers: return FetchState::Requesting;
    case Stage::Body: return FetchState::Reading;
    case Stage::Done: return FetchState::Complete;
    case Stage::Failed: return FetchState::Failed;
  }
  return FetchState::Idle;
}

bool busy() {
  return g.stage == Stage::Clock || g.stage == Stage::Connecting ||
         g.stage == Stage::Headers || g.stage == Stage::Body;
}

bool finished() { return g.stage == Stage::Done || g.stage == Stage::Failed; }

WeatherParse parseResult() {
  if (g.stage != Stage::Done) return WeatherParse::Malformed;
  return g.isForecast ? parseForecast(g.body, g.bodyLen, &g.report)
                      : parsePlaceSearch(g.body, g.bodyLen, g.placeList, &g.placeCount);
}

const WeatherPlace *places() { return g.placeList; }

uint8_t placeCount() { return g.placeCount; }

const WeatherReport &report() { return g.report; }

uint16_t httpStatus() { return static_cast<uint16_t>(g.statusCode); }

const char *message() { return g.message; }

const char *body() { return g.body; }

size_t bodyLength() { return g.bodyLen; }

}  
