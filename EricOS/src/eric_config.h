#pragma once

#include <Arduino.h>



namespace EricConfig {
constexpr uint8_t KbdSda = 6;
constexpr uint8_t KbdScl = 7;
constexpr uint8_t KbdAddress = 0x1F;
constexpr uint8_t KbdFifoRegister = 0x09;


constexpr uint8_t KeyBack = 0x08;
constexpr uint8_t KeyEnter = 0x0A;
constexpr uint8_t KeyEsc = 0xB1;
constexpr uint8_t KeyLeft = 0xB4;
constexpr uint8_t KeyUp = 0xB5;
constexpr uint8_t KeyDown = 0xB6;
constexpr uint8_t KeyRight = 0xB7;
constexpr uint8_t KeyDelete = 0xD4;
constexpr uint8_t KeyF1 = 0x81;
constexpr uint8_t KeyJoyUp = 0x01;
constexpr uint8_t KeyJoyDown = 0x02;
constexpr uint8_t KeyJoyLeft = 0x03;
constexpr uint8_t KeyJoyRight = 0x04;
constexpr uint8_t KeyJoyCenter = 0x05;




constexpr uint8_t KeyStateIdle = 0;
constexpr uint8_t KeyStatePressed = 1;
constexpr uint8_t KeyStateHold = 2;
constexpr uint8_t KeyStateReleased = 3;



constexpr uint8_t KeyModAlt = 0xA1;
constexpr uint8_t KeyModLeftShift = 0xA2;
constexpr uint8_t KeyModRightShift = 0xA3;
constexpr uint8_t KeyModSym = 0xA4;
constexpr uint8_t KeyModCtrl = 0xA5;
constexpr uint8_t KeyCapsLock = 0xC1;



constexpr uint16_t ScratchpadCapacity = 560;
constexpr uint8_t ExplorerPageItems = 8;
constexpr uint8_t ExplorerNameLength = 31;



constexpr uint8_t ExplorerInputLength = 127;



constexpr uint8_t SdMiso = 16;
constexpr uint8_t SdCs = 17;
constexpr uint8_t SdSck = 18;
constexpr uint8_t SdMosi = 19;
constexpr uint8_t SdDetect = 22;





constexpr uint8_t AudioLeft = 26;
constexpr uint8_t AudioRight = 27;
}
