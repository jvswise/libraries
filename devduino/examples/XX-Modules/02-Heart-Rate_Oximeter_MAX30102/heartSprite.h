#ifndef DEVDUINO_HEART_SPRITE
#define DEVDUINO_HEART_SPRITE

#include "DevduinoSprite.h"

static const uint8_t heartBuffer[1][7] PROGMEM = {
{0x60, 0xF0, 0xF8, 0x7C, 0xF8, 0xF0, 0x60}
};

DevduinoSprite heartSprite((uint8_t**) heartBuffer, 7, 1);

#endif //DEVDUINO_C:\FAKEPATH\HEART-BLACK-SHAPE-FOR-VALENTINES_318-46812SPRITE
