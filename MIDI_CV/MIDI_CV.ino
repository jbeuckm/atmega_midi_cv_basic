#include <MIDI.h>
#include <AH_MCP4922.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include "notebook.hpp"

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
double currentFrequencyCode;
double frequencyCodeStep = 0;
double portamentoStepCount = 1;
int lastPitch = 0;

int pitchbendOffset = 0;

const uint16_t frequency[128] PROGMEM = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};


MIDI_CREATE_DEFAULT_INSTANCE();
byte selectedChannel = 17;

void processNote(char pitch, char velocity) {
  targetFrequencyCode = (pitch - 12) * 42;
  frequencyCodeStep = (targetFrequencyCode - currentFrequencyCode) / portamentoStepCount;
  
  pitchDAC.setValue((int)targetFrequencyCode + pitchbendOffset);
  velocityDAC.setValue(velocity * 32);

  tone(TONE_PIN, (unsigned int)pgm_read_word(&frequency[pitch]) + (pitchbendOffset >> 5));
  lastPitch = pitch;
}

Note *result;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  result = notebook.noteOn(pitch, velocity);
  processNote(result->pitch, result->velocity);
  digitalWrite(GATE_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
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
      portamentoStepCount = 1 + value * value;
      break;
        
    case CC_NOTE_PRIORITY:
      if (value > 64) {
        notebook.setMode(velocity);
      } else if (value > 32) {
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
  pitchDAC.setValue((int)targetFrequencyCode + pitchbendOffset);

  tone(TONE_PIN, (unsigned int)pgm_read_word(&frequency[lastPitch]) + (pitchbendOffset >> 5));
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

void setupTimer1khz() {
    //set timer1 interrupt at 1kHz
    cli();
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1  = 0; //initialize counter value to 0;
    // set timer count for 1khz increments
    OCR1A = 1999;// = (16*10^6) / (1000*8) - 1
    TCCR1B |= (1 << WGM12); // turn on CTC mode    
    TCCR1B |= (1 << CS11); // Set CS11 bit for 8 prescaler
    TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
    sei();
}

// 1khz interrupt handler
ISR(TIMER1_COMPA_vect) {
  pitchDAC.setValue((int)targetFrequencyCode + pitchbendOffset);
}

// -----------------------------------------------------------------------------

void setup()
{
//    setupTimer1khz();
  
    selectedChannel = EEPROM.read(0);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(GATE_PIN, OUTPUT);
    digitalWrite(GATE_PIN, LOW);

    delay(500);

    playScale(selectedChannel);

    // calibrate 8V
    targetFrequencyCode = (108 - 12) * 42;
    // calibrate full velocity
    velocityDAC.setValue(32 * 127);

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
}

