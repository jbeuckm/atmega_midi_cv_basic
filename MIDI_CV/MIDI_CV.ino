#include <MIDI.h>
#include "AH_MCP4922.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include "notebook.hpp"
#include "digitalWriteFast.h"

NoteBook notebook;

#define LED_PIN 9
#define GATE_PIN 4

#define TONE_PIN 7

#define CC_ALL_NOTES_OFF 123
#define CC_NOTE_PRIORITY 80
#define CC_PORTAMENTO_TIME 5

byte deviceID = 1;

AH_MCP4922 pitchDAC(10,11,12,LOW,LOW);
AH_MCP4922 velocityDAC(10,11,12,HIGH,LOW);

double targetFrequencyCode;
double currentFrequencyCode = 1;
double frequencyCodeSlideRate = 0;
double portamentoTime = 1;
int lastPitch = 0;
volatile uint16_t toneFrequency;
unsigned long noteStartTime;

int pitchbendOffset = 0;

const uint16_t frequency[128] PROGMEM = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};


MIDI_CREATE_DEFAULT_INSTANCE();
byte selectedChannel = 17;

void processNote(char pitch, char velocity) {
  toneFrequency = (unsigned int)pgm_read_word(&frequency[pitch]);

  targetFrequencyCode = (pitch - 12) * 42;
  frequencyCodeSlideRate = (targetFrequencyCode - currentFrequencyCode) / portamentoTime;
  
  velocityDAC.setValue((int)velocity << 5);

  tone(TONE_PIN, toneFrequency + (pitchbendOffset >> 5));
  lastPitch = pitch;
}

Note *result;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  noteStartTime = millis();
  result = notebook.noteOn(pitch, velocity);
  processNote(result->pitch, result->velocity);
  digitalWrite(GATE_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
}

void updateFrequencyDAC() {

  currentFrequencyCode += frequencyCodeSlideRate * (millis() - noteStartTime);

  if (frequencyCodeSlideRate < 0 && currentFrequencyCode <= targetFrequencyCode) {
    frequencyCodeSlideRate = 0;
    currentFrequencyCode = targetFrequencyCode;
  }
  if (frequencyCodeSlideRate > 0 && currentFrequencyCode >= targetFrequencyCode) {
    frequencyCodeSlideRate = 0;
    currentFrequencyCode = targetFrequencyCode;
  }

  pitchDAC.setValue((int)currentFrequencyCode + pitchbendOffset);  
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  result = notebook.noteOff(pitch);
  
  if (result == 0) {
    digitalWrite(GATE_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
  } else {
    processNote(result->pitch, result->velocity);
  }
}


void handleControlChange(byte channel, byte number, byte value)
{
  switch (number) {

    case CC_PORTAMENTO_TIME:
      portamentoTime = 1.0 + 4.0 * pow(value, 3.5);
      break;
        
    case CC_NOTE_PRIORITY:
      if (value >= 64) {
        notebook.setMode(velocity);
      } else if (value >= 32) {
        notebook.setMode(highest);
      } else {
        notebook.setMode(lowest);
      }
      break;
        
    case CC_ALL_NOTES_OFF:
      notebook.allNotesOff();
      handlePitchBend(selectedChannel, 0);
      digitalWrite(GATE_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      break;
  }
}


void handlePitchBend(byte channel, int bend)
{
  pitchbendOffset = bend >> 4;

  tone(TONE_PIN, toneFrequency + (pitchbendOffset >> 5));
}


void setMidiChannel(byte newChannel) {
  newChannel = newChannel % 17;
  if (newChannel == 0) {
    newChannel = 1;
  }
  EEPROM.update(0, newChannel);
  MIDI.begin(newChannel);
  playScale(newChannel);
  
}


void handleSystemExclusive(byte message[], unsigned size) {
  
  if (message[1] != 0x77) return;      // manufacturer ID
  if (message[2] != 0x16) return;      // model ID
  if (message[3] != deviceID) return;  // device ID 
  
  switch (message[4]) {
    
    case 0x00:
      setMidiChannel(message[5]);
      break;
    
    default:
      break;
  }

}


// -----------------------------------------------------------------------------

void setup()
{
    selectedChannel = EEPROM.read(0);
    if (selectedChannel > 16) {
      selectedChannel = 1;
      EEPROM.update(0, selectedChannel);
    }

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(GATE_PIN, OUTPUT);
    digitalWrite(GATE_PIN, LOW);

    delay(500);

    playScale(selectedChannel);

    // calibrate 8V
    // calibrate full velocity
    handleNoteOn(selectedChannel, 108, 127);
    handleNoteOff(selectedChannel, 108, 127);

    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandlePitchBend(handlePitchBend);
    MIDI.setHandleControlChange(handleControlChange);
    MIDI.setHandleSystemExclusive(handleSystemExclusive);

    setMidiChannel(selectedChannel);
}

void playScale(int channel) {

  int note = 60;

  for (int i=0; i<channel; i++) {

      handleNoteOn(channel, note, 100);
      delay(50);
      handleNoteOff(channel, note, 100);
      delay(50);
      note++;
  }

}

void loop()
{
    MIDI.read();
    updateFrequencyDAC();
}

