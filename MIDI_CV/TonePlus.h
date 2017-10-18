
#ifndef TonePlus_h
  #define TonePlus_h

  #if defined(ARDUINO) && ARDUINO >= 100
    #include <Arduino.h>
  #else
    #include <WProgram.h>
  #endif

  #if defined(__AVR_ATmega8__) || defined(__AVR_ATmega128__)
    #define TIMSK1 TIMSK
  #endif

  void TonePlus(uint8_t pin, unsigned long frequency, void (*sideJob)());
  void updateToneFrequency(unsigned long frequency);
#endif