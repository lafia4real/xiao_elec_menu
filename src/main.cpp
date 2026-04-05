#include <Arduino.h>

#include "board_config.h"
#include "servo.h"
#include "tft.h"

namespace {

String normalizeCommand(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    return cmd;
}

void dispatchGestureLabel(const String &label) {
    tftOnGestureRecognized(label);
}

void handleUartCommand(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);

    if (cmd.length() == 0) {
        return;
    }

    Serial.print("[UART1 RECV] ");
    Serial.println(cmd);

    if (cmd == "LEFT") {
        Serial.println("[ACTION] LEFT -> menu up");
        dispatchGestureLabel("rock");
    } else if (cmd == "RIGHT") {
        Serial.println("[ACTION] RIGHT -> menu down");
        dispatchGestureLabel("paper");
    } else if (cmd == "OK") {
        Serial.println("[ACTION] OK -> open / back");
        dispatchGestureLabel("scissors");
    } else {
        Serial.print("[WARN] Unknown UART1 command: ");
        Serial.println(cmd);
    }
}

void pollUart1() {
    if (!Serial1.available()) {
        return;
    }

    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();
    handleUartCommand(cmd);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, PINS.uartRxPin, PINS.uartTxPin);
    delay(1000);
    Serial.println("Gesture order menu start");
    Serial.print("Target board: ");
    Serial.println(PINS.boardName);
    Serial.println("Type rock / paper / scissors in Serial Monitor to test.");
    Serial.print("UART1 ready on RX=");
    Serial.print(PINS.uartRxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uartTxPin);
    Serial.println("Send LEFT / RIGHT / OK on UART1 to control the menu.");

    servoSetup();
    tftSetup();
}

void loop() {
    uint32_t nowMs = millis();

    pollUart1();
    servoUpdate(nowMs);
    tftUpdate(nowMs);

    if (tftConsumeDetailEntryEvent()) {
        servoStartSequence(nowMs);
    }
}
