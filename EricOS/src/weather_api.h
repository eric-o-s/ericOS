#pragma once

#include <Arduino.h>

#include "weather_data.h"













namespace EricWeather {
enum class FetchState : uint8_t {
  Idle,      
  Connecting, 
  Requesting, 
  Reading,    
  Complete,   
  Failed      
};






constexpr size_t WEATHER_BODY_CAPACITY = 4096;



constexpr uint32_t WEATHER_CONNECT_TIMEOUT_MS = 4000;
constexpr uint32_t WEATHER_READ_IDLE_TIMEOUT_MS = 6000;





constexpr uint32_t WEATHER_CLOCK_TIMEOUT_MS = 8000;


void search(const char *city);


void fetchForecast(const WeatherPlace &place);



void pump();

void abort();


void forgetAnchors();

FetchState state();
bool busy();
bool finished();      
WeatherParse parseResult();
const char *message(); 
const char *body();    
size_t bodyLength();



const WeatherPlace *places();
uint8_t placeCount();




const WeatherReport &report();

uint16_t httpStatus();
}  
