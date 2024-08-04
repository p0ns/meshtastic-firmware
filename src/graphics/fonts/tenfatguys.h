#ifndef TENFATGUYS_h
#define TENFATGUYS_h

#ifdef ARDUINO
#include <Arduino.h>
#elif __MBED__
#define PROGMEM
#endif

extern const uint8_t TenFatGuys_16[] PROGMEM;
extern const uint8_t TenFatGuys_22[] PROGMEM;
extern const uint8_t TenFatGuys_24[] PROGMEM;

#endif