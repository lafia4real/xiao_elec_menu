#include "servo.h"

#include <ESP32Servo.h>

#include "board_config.h"

namespace {

enum class ServoPhase {
    Idle,
    MoveToLeft,
    HoldLeft,
    MoveToRight,
    HoldRight,
    MoveToCenter,
    HoldCenter,
};

Servo g_servo;
ServoPhase g_phase = ServoPhase::Idle;
int g_currentAngle = CENTER_ANGLE;
uint32_t g_lastStepMs = 0;
uint32_t g_phaseStartMs = 0;

void enterPhase(ServoPhase nextPhase, uint32_t nowMs) {
    g_phase = nextPhase;
    g_phaseStartMs = nowMs;

    switch (g_phase) {
        case ServoPhase::MoveToLeft:
            Serial.println("Move to 60 deg");
            break;
        case ServoPhase::MoveToRight:
            Serial.println("Move to 120 deg");
            break;
        case ServoPhase::MoveToCenter:
            Serial.println("Back to center");
            break;
        case ServoPhase::Idle:
        case ServoPhase::HoldLeft:
        case ServoPhase::HoldRight:
        case ServoPhase::HoldCenter:
            break;
    }
}

}  // namespace

void servoSetup() {
    g_servo.setPeriodHertz(50);
    g_servo.attach(PINS.servoPin, 500, 2400);
    g_servo.write(CENTER_ANGLE);
    g_currentAngle = CENTER_ANGLE;

    uint32_t nowMs = millis();
    g_lastStepMs = nowMs;
    g_phaseStartMs = nowMs;
}

void servoStartSequence(uint32_t nowMs) {
    g_currentAngle = CENTER_ANGLE;
    g_servo.write(g_currentAngle);
    g_lastStepMs = nowMs;
    enterPhase(ServoPhase::MoveToLeft, nowMs);
}

void servoUpdate(uint32_t nowMs) {
    if (g_phase == ServoPhase::Idle) {
        return;
    }
    if (nowMs - g_lastStepMs < SERVO_STEP_INTERVAL_MS) {
        return;
    }

    g_lastStepMs = nowMs;

    switch (g_phase) {
        case ServoPhase::MoveToLeft:
            if (g_currentAngle > LEFT_ANGLE) {
                g_currentAngle--;
                g_servo.write(g_currentAngle);
            } else {
                enterPhase(ServoPhase::HoldLeft, nowMs);
            }
            break;

        case ServoPhase::HoldLeft:
            if (nowMs - g_phaseStartMs >= HOLD_LEFT_MS) {
                enterPhase(ServoPhase::MoveToRight, nowMs);
            }
            break;

        case ServoPhase::MoveToRight:
            if (g_currentAngle < RIGHT_ANGLE) {
                g_currentAngle++;
                g_servo.write(g_currentAngle);
            } else {
                enterPhase(ServoPhase::HoldRight, nowMs);
            }
            break;

        case ServoPhase::HoldRight:
            if (nowMs - g_phaseStartMs >= HOLD_RIGHT_MS) {
                enterPhase(ServoPhase::MoveToCenter, nowMs);
            }
            break;

        case ServoPhase::MoveToCenter:
            if (g_currentAngle > CENTER_ANGLE) {
                g_currentAngle--;
                g_servo.write(g_currentAngle);
            } else if (g_currentAngle < CENTER_ANGLE) {
                g_currentAngle++;
                g_servo.write(g_currentAngle);
            } else {
                enterPhase(ServoPhase::HoldCenter, nowMs);
            }
            break;

        case ServoPhase::HoldCenter:
            if (nowMs - g_phaseStartMs >= HOLD_CENTER_MS) {
                enterPhase(ServoPhase::Idle, nowMs);
            }
            break;

        case ServoPhase::Idle:
            break;
    }
}
