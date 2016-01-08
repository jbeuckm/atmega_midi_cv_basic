#include <MIDI.h>
#include "AH_MCP4922.h"

#define LED 13   		    // LED pin on Arduino Uno

#define GATE_PIN 3

AH_MCP4922 AnalogOutput1(11,10,12,LOW,LOW);
AH_MCP4922 AnalogOutput2(11,10,12,HIGH,LOW);

int liveNoteCount = 0;
int pitchbendOffset = 0;
int baseNoteFrequency;

MIDI_CREATE_DEFAULT_INSTANCE();

byte selectedChannel = 17;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (selectedChannel == 17) {
    selectedChannel = channel;
  }
  else if (channel != selectedChannel) {
    return;
  }
  
  liveNoteCount++;
  
  baseNoteFrequency = (pitch - 12) * 42;
  AnalogOutput1.setValue(baseNoteFrequency + pitchbendOffset);
  AnalogOutput2.setValue(velocity * 32);

  digitalWrite(GATE_PIN, HIGH);
  digitalWrite(LED, HIGH);
 }

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (channel != selectedChannel) {
    return;
  }
  liveNoteCount--;
  
  if (liveNoteCount == 0) {
    digitalWrite(GATE_PIN, LOW);
    digitalWrite(LED, LOW);
  }
}


void handleControlChange(byte channel, byte number, byte value)
{
  if (channel != selectedChannel) {
    return;
  }

}


void handlePitchBend(byte channel, int bend)
{
  pitchbendOffset = bend >> 4;

  AnalogOutput1.setValue(baseNoteFrequency + pitchbendOffset);
}


// -----------------------------------------------------------------------------

void setup()
{
    int channelSpan = 1024 / 16;
    int channelInput = analogRead(0);
    selectedChannel = channelInput / channelSpan;

    Serial.begin(115200);
    Serial.println(channelInput);
    Serial.println(selectedChannel);
    
    pinMode(LED, OUTPUT);
    pinMode(GATE_PIN, OUTPUT);

    delay(1000);

    playScale(selectedChannel);

    // calibrate 8V
    baseNoteFrequency = (108 - 12) * 42;
    AnalogOutput1.setValue(baseNoteFrequency);
    // calibrate full velocity
//    AnalogOutput2.setValue(32 * 127);

    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandlePitchBend(handlePitchBend);
    MIDI.begin(selectedChannel);
}


void playScale(int channel) {

  int note = 36;

  for (int i=0; i<channel; i++) {
      handleNoteOn(channel, note, 127);
      delay(150);
      handleNoteOff(channel, note, 127);
      delay(150);

      note ++;
  }

}


int state = 0;

void loop()
{
    MIDI.read();
}

