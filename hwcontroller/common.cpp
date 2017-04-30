#include "common.h"

#include <Arduino.h>

#if DEBUG
void __assert(bool success, String msg) {
    if (!success) {
        Serial.println(msg);
        Serial.flush();
        abort();
    }
}
#endif

uint8_t requestedAction = EA_NONE;
uint8_t currentAction = EA_NONE;
bool hasPriority = false;

RoadInfo crossroad[3];
