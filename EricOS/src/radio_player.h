#pragma once



namespace EricRadio {
enum class State : unsigned char { Idle, Connecting, Playing, Failed };
constexpr unsigned char StationCount = 4;

bool begin();
bool play(unsigned char stationIndex);
bool start(unsigned char stationIndex);
void pump();
void stop();
State state();
unsigned char station();
const char *stationName(unsigned char stationIndex);
const char *stationGenre(unsigned char stationIndex);
const char *message();
}
