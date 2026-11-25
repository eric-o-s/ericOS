#pragma once

#include <Arduino.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputPWM.h>










namespace EricAudioEngine {

enum class Owner : uint8_t { None, LocalFile, Radio, Synth };

AudioGeneratorMP3 &decoder();
AudioOutputPWM &output();
AudioOutputPWM &synthOutput();


bool prepareOutput();
bool prepareSynthOutput();


bool outputRunning();
bool synthOutputRunning();






void acquire(Owner who);



void release(Owner who);

bool isOwner(Owner who);

}
