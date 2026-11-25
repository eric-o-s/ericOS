#include "weather_data.h"

#include <string.h>









namespace {




struct Reader {
  const char *data;
  size_t length;
  size_t pos;
};

bool atEnd(const Reader &r) { return r.pos >= r.length; }

char peek(const Reader &r) { return r.pos < r.length ? r.data[r.pos] : '\0'; }

bool expect(Reader &r, char c) {
  if (atEnd(r) || r.data[r.pos] != c) return false;
  ++r.pos;
  return true;
}



void skipSpace(Reader &r) {
  while (!atEnd(r)) {
    const char c = r.data[r.pos];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++r.pos;
    else break;
  }
}



bool seekKey(Reader &r, const char *key) {
  const size_t keyLength = strlen(key);
  const size_t start = r.pos;
  while (!atEnd(r)) {
    if (r.data[r.pos] == '"') {
      const size_t nameStart = r.pos + 1;
      size_t nameEnd = nameStart;
      while (nameEnd < r.length && r.data[nameEnd] != '"') {
        if (r.data[nameEnd] == '\\') ++nameEnd;  
        ++nameEnd;
      }
      if (nameEnd >= r.length) break;
      if (nameEnd - nameStart == keyLength && memcmp(r.data + nameStart, key, keyLength) == 0) {
        r.pos = nameEnd + 1;
        skipSpace(r);
        if (expect(r, ':')) return true;
        r.pos = nameEnd + 1;
        continue;
      }
      r.pos = nameEnd + 1;
      continue;
    }
    ++r.pos;
  }
  r.pos = start;
  return false;
}

bool readString(Reader &r, char *out, size_t outSize) {
  if (outSize == 0) return false;
  out[0] = '\0';
  skipSpace(r);
  if (!expect(r, '"')) return false;
  size_t written = 0;
  while (!atEnd(r)) {
    char c = r.data[r.pos++];
    if (c == '"') {
      out[written] = '\0';
      return true;
    }
    if (c == '\\') {
      if (atEnd(r)) break;
      const char escaped = r.data[r.pos++];
      switch (escaped) {
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 'r': c = '\r'; break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'u': {
          
          
          
          if (r.pos + 4 > r.length) return false;
          char hex[5] = {r.data[r.pos], r.data[r.pos + 1], r.data[r.pos + 2], r.data[r.pos + 3], '\0'};
          r.pos += 4;
          const long code = strtol(hex, nullptr, 16);
          out[written < outSize - 1 ? written++ : outSize - 1] =
              (code < 256) ? static_cast<char>(code) : '?';
          continue;
        }
        default: c = escaped; break;
      }
    }
    if (written + 1 < outSize) out[written++] = c;
  }
  out[written < outSize ? written : outSize - 1] = '\0';
  return false;
}


bool readFloat(Reader &r, float *out) {
  skipSpace(r);
  const size_t start = r.pos;
  if (!atEnd(r) && (r.data[r.pos] == '-' || r.data[r.pos] == '+')) ++r.pos;
  bool digits = false;
  while (!atEnd(r)) {
    const char c = r.data[r.pos];
    if (c >= '0' && c <= '9') { digits = true; ++r.pos; continue; }
    if (c == '.' || c == 'e' || c == 'E' || c == '-' || c == '+') { ++r.pos; continue; }
    break;
  }
  if (!digits) return false;
  char scratch[32];
  const size_t n = r.pos - start;
  if (n >= sizeof(scratch)) return false;
  memcpy(scratch, r.data + start, n);
  scratch[n] = '\0';
  *out = strtof(scratch, nullptr);
  return true;
}

bool readInt(Reader &r, int32_t *out) {
  float value = 0.0f;
  if (!readFloat(r, &value)) return false;
  *out = static_cast<int32_t>(value < 0.0f ? value - 0.5f : value + 0.5f);
  return true;
}



bool skipValue(Reader &r) {
  skipSpace(r);
  if (atEnd(r)) return false;
  const char c = r.data[r.pos];
  if (c == '"') {
    char scratch[8];
    return readString(r, scratch, sizeof(scratch)) || r.pos < r.length;
  }
  if (c == '{' || c == '[') {
    const char close = (c == '{') ? '}' : ']';
    int depth = 0;
    while (!atEnd(r)) {
      const char d = r.data[r.pos];
      if (d == '"') {
        char scratch[8];
        readString(r, scratch, sizeof(scratch));
        continue;
      }
      if (d == '{' || d == '[') ++depth;
      if (d == '}' || d == ']') {
        --depth;
        ++r.pos;
        if (depth == 0) return d == close;
        continue;
      }
      ++r.pos;
    }
    return false;
  }
  while (!atEnd(r)) {
    const char d = r.data[r.pos];
    if (d == ',' || d == '}' || d == ']') break;
    ++r.pos;
  }
  return true;
}

bool enterObject(Reader &r) {
  skipSpace(r);
  return expect(r, '{');
}

void copyIfPresent(char *dst, size_t dstSize, const char *src) {
  if (!src || !src[0]) {
    if (dstSize) dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}




template <typename Sink>
bool readNumberArray(Reader &r, Sink sink, uint8_t limit) {
  if (!expect(r, '[')) return false;
  uint8_t index = 0;
  while (!atEnd(r) && index < limit) {
    skipSpace(r);
    if (peek(r) == ',') { ++r.pos; continue; }
    
    
    
    
    if (peek(r) == ']') { ++r.pos; return true; }
    float value = 0.0f;
    if (!readFloat(r, &value)) return index > 0;
    sink(value, index);
    ++index;
  }
  
  while (!atEnd(r)) {
    skipSpace(r);
    if (peek(r) == ']') { ++r.pos; return true; }
    if (peek(r) == ',') { ++r.pos; continue; }
    if (!skipValue(r)) return false;
  }
  return false;
}


template <typename Sink>
bool readStringArray(Reader &r, Sink sink, uint8_t limit) {
  if (!expect(r, '[')) return false;
  uint8_t index = 0;
  while (!atEnd(r) && index < limit) {
    skipSpace(r);
    if (peek(r) == ',') { ++r.pos; continue; }
    
    
    
    
    if (peek(r) == ']') { ++r.pos; return true; }
    char value[24];
    if (!readString(r, value, sizeof(value))) return index > 0;
    sink(value, index);
    ++index;
  }
  while (!atEnd(r)) {
    skipSpace(r);
    if (peek(r) == ']') { ++r.pos; return true; }
    if (peek(r) == ',') { ++r.pos; continue; }
    if (!skipValue(r)) return false;
  }
  return false;
}




int16_t toTenths(float value) {
  const float scaled = value * 10.0f;
  if (scaled > 32767.0f) return 32767;
  if (scaled < -32768.0f) return -32768;
  return static_cast<int16_t>(scaled < 0.0f ? scaled - 0.5f : scaled + 0.5f);
}

}  




const char *weatherCodeLabel(uint8_t code) {
  if (code == 0) return "CLEAR SKY";
  if (code == 1) return "MAINLY CLEAR";
  if (code == 2) return "PARTLY CLOUDY";
  if (code == 3) return "OVERCAST";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 57) return "DRIZZLE";
  if (code >= 61 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "RAIN SHOWERS";
  if (code == 85 || code == 86) return "SNOW SHOWERS";
  if (code >= 95) return "THUNDERSTORM";
  return "UNKNOWN";
}


const char *weatherCodeShort(uint8_t code) {
  if (code == 0) return "CLR";
  if (code == 1) return "CLR";
  if (code == 2) return "PTC";
  if (code == 3) return "OVC";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 57) return "DZL";
  if (code >= 61 && code <= 67) return "RAN";
  if (code >= 71 && code <= 77) return "SNW";
  if (code >= 80 && code <= 82) return "SHR";
  if (code == 85 || code == 86) return "SHS";
  if (code >= 95) return "TST";
  return "???";
}

void weatherSummary(char *out, size_t outSize, const WeatherNow &now) {
  if (!out || outSize == 0) return;
  
  
  
  
  
  
  
  
  
  const bool below = now.temperatureF < 0;
  const int magnitude = below ? -now.temperatureF : now.temperatureF;
  snprintf(out, outSize, "%s %s%d.%dF", weatherCodeLabel(now.code), below ? "-" : "",
           magnitude / 10, magnitude % 10);
}




const char *weatherWeekday(const char *isoDate) {
  static const char *const names[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (!isoDate || strlen(isoDate) < 10) return "---";
  int year = 0, month = 0, day = 0;
  if (sscanf(isoDate, "%4d-%2d-%2d", &year, &month, &day) != 3) return "---";
  if (month < 1 || month > 12 || day < 1 || day > 31) return "---";
  
  static const int offsets[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (year < 1) year = 1;
  int y = year - (month < 3 ? 1 : 0);
  const int weekday = (y + (y / 4) - (y / 100) + (y / 400) + offsets[month - 1] + day) % 7;
  return names[(weekday + 7) % 7];
}

const char *weatherMonthShort(const char *isoDate) {
  static const char *const names[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  if (!isoDate || strlen(isoDate) < 7) return "---";
  int year = 0, month = 0, day = 0;
  if (sscanf(isoDate, "%4d-%2d-%2d", &year, &month, &day) != 3) return "---";
  if (month < 1 || month > 12) return "---";
  return names[month - 1];
}

void weatherUtcOffsetLabel(char *out, size_t outSize, int32_t offsetSeconds) {
  if (!out || outSize == 0) return;
  const char *sign = offsetSeconds < 0 ? "-" : "+";
  int32_t magnitude = offsetSeconds < 0 ? -offsetSeconds : offsetSeconds;
  const int32_t hours = magnitude / 3600;
  const int32_t minutes = (magnitude % 3600) / 60;
  if (minutes) {
    snprintf(out, outSize, "UTC%s%ld:%02ld", sign, static_cast<long>(hours),
             static_cast<long>(minutes));
  } else {
    snprintf(out, outSize, "UTC%s%ld", sign, static_cast<long>(hours));
  }
}




static bool appendPlacePart(char *out, size_t outSize, const char *part) {
  if (!part || part[0] == '\0') return true;
  const size_t used = strlen(out);
  const size_t extra = (used == 0 ? 0 : 2) + strlen(part);
  if (used + extra + 1 > outSize) return false;
  if (used > 0) {
    out[used] = ',';
    out[used + 1] = ' ';
  }
  memcpy(out + used + (used > 0 ? 2 : 0), part, strlen(part) + 1);
  return true;
}

void weatherPlaceLine(char *out, size_t outSize, const WeatherPlace &place, uint8_t maxChars) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  
  
  if (maxChars < 6) return;

  
  
  
  const char *const combinations[4][3] = {
      {place.name, place.region, place.country},
      {place.name, place.region, nullptr},
      {place.name, nullptr, place.country},
      {place.name, nullptr, nullptr},
  };
  for (uint8_t attempt = 0; attempt < 4; ++attempt) {
    char candidate[80];
    candidate[0] = '\0';
    if (!appendPlacePart(candidate, sizeof(candidate), combinations[attempt][0])) break;
    if (!appendPlacePart(candidate, sizeof(candidate), combinations[attempt][1])) continue;
    if (!appendPlacePart(candidate, sizeof(candidate), combinations[attempt][2])) continue;
    if (strlen(candidate) <= maxChars && strlen(candidate) + 1 <= outSize) {
      strcpy(out, candidate);
      return;
    }
  }

  
  
  const char *name = place.name;
  if (name[0] == '\0') return;
  const size_t kept = maxChars - 3;
  if (kept == 0 || kept + 1 > outSize) return;
  strncpy(out, name, kept);
  out[kept] = '\0';
  strncat(out, "...", outSize - strlen(out) - 1);
}

const char *weatherParseMessage(WeatherParse result) {
  switch (result) {
    case WeatherParse::Ok: return "OK";
    case WeatherParse::NoResults: return "NO MATCHING CITY";
    case WeatherParse::Malformed: return "COULD NOT READ WEATHER DATA";
    case WeatherParse::ServerError: return "WEATHER SERVICE ERROR";
  }
  return "WEATHER ERROR";
}

WeatherParse parsePlaceSearch(const char *json, size_t length,
                              WeatherPlace *places, uint8_t *count) {
  *count = 0;
  if (!json || length == 0 || !places) return WeatherParse::Malformed;

  
  
  Reader probe{json, length, 0};
  if (seekKey(probe, "error")) {
    skipSpace(probe);
    if (peek(probe) == 't') return WeatherParse::ServerError;
  }

  Reader reader{json, length, 0};
  
  
  
  if (!seekKey(reader, "results")) return WeatherParse::NoResults;
  skipSpace(reader);
  if (!expect(reader, '[')) return WeatherParse::Malformed;

  while (!atEnd(reader) && *count < WEATHER_MAX_PLACES) {
    skipSpace(reader);
    if (peek(reader) == ',') { ++reader.pos; continue; }
    if (peek(reader) == ']') break;
    if (!enterObject(reader)) break;

    WeatherPlace place{};
    place.utcOffsetSeconds = 0;
    bool sawName = false;

    while (!atEnd(reader)) {
      skipSpace(reader);
      if (peek(reader) == ',') { ++reader.pos; continue; }
      if (peek(reader) == '}') { ++reader.pos; break; }
      char key[32];
      if (!readString(reader, key, sizeof(key))) break;
      skipSpace(reader);
      if (!expect(reader, ':')) break;

      if (strcmp(key, "name") == 0) {
        if (readString(reader, place.name, sizeof(place.name))) sawName = true;
      } else if (strcmp(key, "admin1") == 0) {
        readString(reader, place.region, sizeof(place.region));
      } else if (strcmp(key, "country") == 0) {
        readString(reader, place.country, sizeof(place.country));
      } else if (strcmp(key, "latitude") == 0) {
        readFloat(reader, &place.latitude);
      } else if (strcmp(key, "longitude") == 0) {
        readFloat(reader, &place.longitude);
      } else if (strcmp(key, "utc_offset_seconds") == 0) {
        readInt(reader, &place.utcOffsetSeconds);
      } else if (!skipValue(reader)) {
        break;
      }
    }

    if (sawName) {
      places[*count] = place;
      ++*count;
    }
  }

  if (*count == 0) return WeatherParse::NoResults;
  return WeatherParse::Ok;
}

WeatherParse parseForecast(const char *json, size_t length, WeatherReport *report) {
  if (!json || length == 0 || !report) return WeatherParse::Malformed;
  memset(report, 0, sizeof(*report));
  report->dayCount = 0;
  for (uint8_t i = 0; i < WEATHER_DAYS; ++i) report->days[i].precipitationChance = 255;

  Reader probe{json, length, 0};
  if (seekKey(probe, "error")) {
    skipSpace(probe);
    if (peek(probe) == 't') return WeatherParse::ServerError;
  }

  
  Reader reader{json, length, 0};
  bool sawCurrent = false, sawDaily = false;
  
  
  
  
  
  bool sawTemperature = false, sawDate = false, sawHigh = false;
  copyIfPresent(report->place.name, sizeof(report->place.name), "");
  report->place.latitude = 0.0f;
  report->place.longitude = 0.0f;

  if (!enterObject(reader)) return WeatherParse::Malformed;
  while (!atEnd(reader)) {
    skipSpace(reader);
    if (peek(reader) == ',') { ++reader.pos; continue; }
    if (peek(reader) == '}') break;
    char key[32];
    if (!readString(reader, key, sizeof(key))) break;
    skipSpace(reader);
    if (!expect(reader, ':')) break;

    if (strcmp(key, "latitude") == 0) {
      readFloat(reader, &report->place.latitude);
    } else if (strcmp(key, "longitude") == 0) {
      readFloat(reader, &report->place.longitude);
    } else if (strcmp(key, "utc_offset_seconds") == 0) {
      readInt(reader, &report->place.utcOffsetSeconds);
    } else if (strcmp(key, "timezone") == 0) {
      readString(reader, report->now.timezone, sizeof(report->now.timezone));
    } else if (strcmp(key, "current") == 0) {
      if (!enterObject(reader)) break;
      while (!atEnd(reader)) {
        skipSpace(reader);
        if (peek(reader) == ',') { ++reader.pos; continue; }
        if (peek(reader) == '}') { ++reader.pos; break; }
        char field[32];
        if (!readString(reader, field, sizeof(field))) break;
        skipSpace(reader);
        if (!expect(reader, ':')) break;
        float value = 0.0f;
        if (strcmp(field, "temperature_2m") == 0) {
          if (readFloat(reader, &value)) {
            report->now.temperatureF = toTenths(value);
            sawTemperature = true;
          }
        } else if (strcmp(field, "apparent_temperature") == 0) {
          if (readFloat(reader, &value)) report->now.feelsLikeF = toTenths(value);
        } else if (strcmp(field, "relative_humidity_2m") == 0) {
          if (readFloat(reader, &value)) report->now.humidity = static_cast<uint8_t>(value + 0.5f);
        } else if (strcmp(field, "weather_code") == 0) {
          if (readFloat(reader, &value)) report->now.code = static_cast<uint8_t>(value + 0.5f);
        } else if (strcmp(field, "wind_speed_10m") == 0) {
          if (readFloat(reader, &value)) report->now.windMph = static_cast<uint8_t>(value + 0.5f);
        } else if (strcmp(field, "time") == 0) {
          readString(reader, report->now.observedAt, sizeof(report->now.observedAt));
        } else if (!skipValue(reader)) {
          break;
        }
      }
      sawCurrent = true;
    } else if (strcmp(key, "daily") == 0) {
      if (!enterObject(reader)) break;
      
      
      
      while (!atEnd(reader)) {
        skipSpace(reader);
        if (peek(reader) == ',') { ++reader.pos; continue; }
        if (peek(reader) == '}') { ++reader.pos; break; }
        char field[40];
        if (!readString(reader, field, sizeof(field))) break;
        skipSpace(reader);
        if (!expect(reader, ':')) break;

        if (strcmp(field, "time") == 0) {
          readStringArray(reader, [report, &sawDate](const char *value, uint8_t index) {
            if (index < WEATHER_DAYS) {
              copyIfPresent(report->days[index].date, sizeof(report->days[index].date), value);
              
              
              if (value[0] != '\0') sawDate = true;
            }
            if (index + 1U > report->dayCount) report->dayCount = index + 1U;
          }, WEATHER_DAYS);
        } else if (strcmp(field, "weather_code") == 0) {
          readNumberArray(reader, [report](float value, uint8_t index) {
            if (index < WEATHER_DAYS) report->days[index].code = static_cast<uint8_t>(value + 0.5f);
          }, WEATHER_DAYS);
        } else if (strcmp(field, "temperature_2m_max") == 0) {
          readNumberArray(reader, [report, &sawHigh](float value, uint8_t index) {
            if (index < WEATHER_DAYS) {
              report->days[index].highF = toTenths(value);
              sawHigh = true;
            }
          }, WEATHER_DAYS);
        } else if (strcmp(field, "temperature_2m_min") == 0) {
          readNumberArray(reader, [report](float value, uint8_t index) {
            if (index < WEATHER_DAYS) report->days[index].lowF = toTenths(value);
          }, WEATHER_DAYS);
        } else if (strcmp(field, "precipitation_probability_max") == 0) {
          readNumberArray(reader, [report](float value, uint8_t index) {
            if (index < WEATHER_DAYS) report->days[index].precipitationChance = static_cast<uint8_t>(value + 0.5f);
          }, WEATHER_DAYS);
        } else if (!skipValue(reader)) {
          break;
        }
      }
      sawDaily = true;
    } else if (!skipValue(reader)) {
      break;
    }
  }

  if (!sawCurrent || !sawDaily) return WeatherParse::Malformed;
  if (!sawTemperature) return WeatherParse::Malformed;
  if (report->dayCount == 0 || !sawDate || !sawHigh) return WeatherParse::Malformed;
  return WeatherParse::Ok;
}
