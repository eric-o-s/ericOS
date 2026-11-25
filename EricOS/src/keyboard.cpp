#include <Arduino.h>
#include <Wire.h>
#include "eric_config.h"
#include "keyboard.h"

namespace {




bool shiftLatched = false;
bool ctrlLatched = false;
bool capsLatched = false;

bool isShiftKey(uint8_t code) {
  return code == EricConfig::KeyModLeftShift || code == EricConfig::KeyModRightShift;
}

bool isCtrlKey(uint8_t code) {
  return code == EricConfig::KeyModCtrl;
}

bool isCapsKey(uint8_t code) {
  return code == EricConfig::KeyCapsLock;
}

bool isModifier(uint8_t code) {
  return isShiftKey(code) || isCtrlKey(code) || isCapsKey(code) || code == EricConfig::KeyModAlt || code == EricConfig::KeyModSym;
}




uint8_t shiftedSymbol(uint8_t ch) {
  switch (ch) {
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';
    case '-': return '_';
    case '=': return '+';
    case '[': return '{';
    case ']': return '}';
    case '\\': return '|';
    case ';': return ':';
    case '\'': return '"';
    case ',': return '<';
    case '.': return '>';
    case '/': return '?';
    case '`': return '~';
    default: return ch;
  }
}

} 

int readKeyboardEvent() {
  
  
  
  Wire1.beginTransmission(EricConfig::KbdAddress);
  Wire1.write(EricConfig::KbdFifoRegister);
  if (Wire1.endTransmission(true) != 0) return -1;

  
  
  
  delayMicroseconds(250);
  if (Wire1.requestFrom((int)EricConfig::KbdAddress, 2, true) != 2) return -1;

  
  
  const uint8_t state = Wire1.read();
  const uint8_t keyCode = Wire1.read();
  if (keyCode == 0 || state == EricConfig::KeyStateIdle || state == EricConfig::KeyStateHold) return -1;

  
  
  if (isModifier(keyCode)) {
    const bool down = state == EricConfig::KeyStatePressed;
    if (isShiftKey(keyCode)) shiftLatched = down;
    else if (isCtrlKey(keyCode)) ctrlLatched = down;
    else if (isCapsKey(keyCode)) capsLatched = down;
    return -1;
  }
  if (state != EricConfig::KeyStatePressed) return -1;

  uint8_t ch = keyCode;
  if (ch >= 'a' && ch <= 'z') {
    if (ctrlLatched) ch &= 0x1F;
    else if (shiftLatched != capsLatched) ch &= static_cast<uint8_t>(~0x20);
  } else if (shiftLatched && ch >= 0x20 && ch <= 0x7E) {
    ch = shiftedSymbol(ch);
  }
  return ch;
}
