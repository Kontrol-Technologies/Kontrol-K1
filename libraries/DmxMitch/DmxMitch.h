/**
* DmxMitch - A simple interface to DMX.
*
* Copyright (c) 2017, Designed by Peter Knight hacked to work with post pre-historic Arduino's by Mitch Shuttleworth
*/

#ifndef DmxMitch_h
#define DmxMitch_h

#include <inttypes.h>

#if RAMEND <= 0x4FF
#define DMX_SIZE 128
#else
#define DMX_SIZE 512
#endif

class DmxMitchClass
{
  public:
    void maxChannel(int);
    void write(int, uint8_t);
    void usePin(uint8_t);
};
extern DmxMitchClass DmxMitch;

#endif