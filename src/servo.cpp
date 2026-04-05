#include "servo.h"

#include "board_config.h"

namespace {

constexpr uint32_t ATOMIZER_ON_MS = 5000;
constexpr uint32_t ATOMIZER_PULSE_MS = 200;
constexpr uint32_t ATOMIZER_GAP_MS = 200;

enum class AtomizerPhase {
    Idle,
    PulseOnLow,
    PulseOnHigh,
    HoldOn,
    PulseOffLow,
    PulseOffHigh,
};

AtomizerPhase g_atomizerPhase = AtomizerPhase::Idle;
uint32_t g_phaseStartMs = 0;

}  // namespace

void servoSetup() {
    pinMode(PINS.servoPin, OUTPUT);
    digitalWrite(PINS.servoPin, HIGH);
}

void servoStartSequence(uint32_t nowMs) {
    g_atomizerPhase = AtomizerPhase::PulseOnLow;
    g_phaseStartMs = nowMs;
    digitalWrite(PINS.servoPin, LOW);
    Serial.println("Atomizer press ON");
}

void servoUpdate(uint32_t nowMs) {
    switch (g_atomizerPhase) {
        case AtomizerPhase::Idle:
            return;

        case AtomizerPhase::PulseOnLow:
            if (nowMs - g_phaseStartMs >= ATOMIZER_PULSE_MS) {
                digitalWrite(PINS.servoPin, HIGH);
                g_atomizerPhase = AtomizerPhase::PulseOnHigh;
                g_phaseStartMs = nowMs;
                Serial.println("Atomizer release ON");
            }
            break;

        case AtomizerPhase::PulseOnHigh:
            if (nowMs - g_phaseStartMs >= ATOMIZER_GAP_MS) {
                g_atomizerPhase = AtomizerPhase::HoldOn;
                g_phaseStartMs = nowMs;
                Serial.println("Atomizer hold ON for 5 seconds");
            }
            break;

        case AtomizerPhase::HoldOn:
            if (nowMs - g_phaseStartMs >= ATOMIZER_ON_MS) {
                digitalWrite(PINS.servoPin, LOW);
                g_atomizerPhase = AtomizerPhase::PulseOffLow;
                g_phaseStartMs = nowMs;
                Serial.println("Atomizer press OFF");
            }
            break;

        case AtomizerPhase::PulseOffLow:
            if (nowMs - g_phaseStartMs >= ATOMIZER_PULSE_MS) {
                digitalWrite(PINS.servoPin, HIGH);
                g_atomizerPhase = AtomizerPhase::PulseOffHigh;
                g_phaseStartMs = nowMs;
                Serial.println("Atomizer release OFF");
            }
            break;

        case AtomizerPhase::PulseOffHigh:
            if (nowMs - g_phaseStartMs >= ATOMIZER_GAP_MS) {
                g_atomizerPhase = AtomizerPhase::Idle;
                g_phaseStartMs = nowMs;
                Serial.println("Atomizer idle HIGH");
            }
            break;
    }
}

bool servoIsBusy() {
    return g_atomizerPhase != AtomizerPhase::Idle;
}
