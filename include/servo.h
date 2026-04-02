#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <Arduino.h>

void servoSetup();
void servoUpdate(uint32_t nowMs);
void servoStartSequence(uint32_t nowMs);

#endif
