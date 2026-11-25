#pragma once

#include <Arduino.h>












namespace EricTransfer {



enum class State : uint8_t { Off, Ready, Receiving, Complete, Error };


enum class Direction : uint8_t { Idle, Incoming, Outgoing };
bool start();
void stop();


void shutdownHotspot();
void pump();



void maintenance();
State state();
Direction direction();
const char *message();
const char *ipAddress();


bool hotspotUp();
uint32_t receivedBytes();
uint32_t sentBytes();





uint32_t progressDone();
uint32_t progressTotal();
}
