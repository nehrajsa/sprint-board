#pragma once

#ifdef ESP32

#include <stdint.h>

// Direction written by controller task (core 1), read by plugin task (core 0).
// uint8_t writes are atomic on ESP32 — volatile is sufficient.
extern volatile uint8_t controllerDirection; // 1=up 2=right 3=down 4=left 0=none

void initController();
void updateController();
void enableControllerPairing();

#endif
