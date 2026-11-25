#pragma once

#include <Arduino.h>












constexpr uint8_t WEATHER_MAX_PLACES = 5;
constexpr uint8_t WEATHER_DAYS = 7;

enum class WeatherParse : uint8_t {
  Ok,
  NoResults,     
  Malformed,     
  ServerError,   
};

const char *weatherParseMessage(WeatherParse result);

struct WeatherPlace {
  char name[28];
  char region[20];  
  char country[20];
  float latitude;
  float longitude;
  int32_t utcOffsetSeconds;
};

struct WeatherDay {
  char date[12];          
  uint8_t code;           
  int16_t highF;          
  int16_t lowF;
  uint8_t precipitationChance;  
};

struct WeatherNow {
  int16_t temperatureF;   
  int16_t feelsLikeF;
  uint8_t humidity;       
  uint8_t code;           
  uint8_t windMph;
  char observedAt[18];    
  char timezone[28];
};

struct WeatherReport {
  WeatherNow now;
  WeatherDay days[WEATHER_DAYS];
  uint8_t dayCount;
  WeatherPlace place;     
};



WeatherParse parsePlaceSearch(const char *json, size_t length,
                              WeatherPlace *places, uint8_t *count);



WeatherParse parseForecast(const char *json, size_t length, WeatherReport *report);



const char *weatherCodeLabel(uint8_t code);
const char *weatherCodeShort(uint8_t code);


void weatherSummary(char *out, size_t outSize, const WeatherNow &now);




const char *weatherWeekday(const char *isoDate);
const char *weatherMonthShort(const char *isoDate);


void weatherUtcOffsetLabel(char *out, size_t outSize, int32_t offsetSeconds);












void weatherPlaceLine(char *out, size_t outSize, const WeatherPlace &place, uint8_t maxChars);