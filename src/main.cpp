#include <Arduino.h>

#include "board_config.h"
#include "servo.h"
#include "tft.h"

void onGestureRecognized(const String &label) {
    tftOnGestureRecognized(label);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Gesture order menu start");
    Serial.print("Target board: ");
    Serial.println(PINS.boardName);
    Serial.println("Type rock / paper / scissors in Serial Monitor to test.");

    servoSetup();
    tftSetup();
}

void loop() {
    uint32_t nowMs = millis();

    servoUpdate(nowMs);
    tftUpdate(nowMs);

    if (tftConsumeDetailEntryEvent()) {
        servoStartSequence(nowMs);
    }
}
