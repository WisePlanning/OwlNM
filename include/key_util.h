#ifndef _KEY_UTIL_
#define _KEY_UTIL_

#include "config.h"

#define UNKNOWN_KEY "\0"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/* check if event is shift key */
bool isShift(uint16_t code);


char *getKeyText(uint16_t code, uint8_t shift_pressed);

#endif
