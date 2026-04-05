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

void handleScreenCommand(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);

    if (cmd.length() == 0) {
        return;
    }

    if (cmd == "1") {
        Serial.println("[ACTION] SCREEN 1 -> send to UART2");
        Serial2.print("1\n");
    } else if (cmd == "2") {
        Serial.println("[ACTION] SCREEN 2 -> send to UART2");
        Serial2.print("2\n");
    } else {
        Serial.print("[WARN] Unknown screen command: ");
        Serial.println(cmd);
    }
}

void handleUartCommand(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);
    uint32_t nowMs = millis();

    if (cmd.length() == 0) {
        return;
    }

    Serial.print("[UART1 RECV] ");
    Serial.println(cmd);

    if (cmd == "START") {
        Serial.println("[ACTION] START -> open / keep alive");
        tftOnStartSignal(nowMs);
    } else if (cmd == "LEFT") {
        Serial.println("[ACTION] LEFT -> menu up");
        dispatchGestureLabel("rock");
    } else if (cmd == "RIGHT") {
        Serial.println("[ACTION] RIGHT -> menu down");
        dispatchGestureLabel("paper");
    } else if (cmd == "OK") {
        Serial.println("[ACTION] OK -> open detail");
        dispatchGestureLabel("scissors");
    } else {
        Serial.print("[WARN] Unknown UART1 command: ");
        Serial.println(cmd);
    }
}

void handleUart2Command(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);

    if (cmd.length() == 0) {
        return;
    }

    Serial.print("[UART2 LOG] ");
    Serial.println(cmd);
}

void pollUart1() {
    if (!Serial1.available()) {
        return;
    }

    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();
    handleUartCommand(cmd);
}

void pollUart2() {
    if (!Serial2.available()) {
        return;
    }

    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    handleUart2Command(cmd);
}

void pollUsbScreenCommands() {
    if (!Serial.available()) {
        return;
    }

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "1" || cmd == "2") {
        Serial.print("[USB SCREEN RECV] ");
        Serial.println(cmd);
        handleScreenCommand(cmd);
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, PINS.uartRxPin, PINS.uartTxPin);
    Serial2.begin(115200, SERIAL_8N1, PINS.uart2RxPin, PINS.uart2TxPin);
    delay(1000);
    Serial.println("Gesture order menu start");
    Serial.print("Target board: ");
    Serial.println(PINS.boardName);
    Serial.println("Type rock / paper / scissors in Serial Monitor to test.");
    Serial.println("Type 1 / 2 in Serial Monitor to test screen switching.");
    Serial.print("UART1 ready on RX=");
    Serial.print(PINS.uartRxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uartTxPin);
    Serial.println("Send START / LEFT / RIGHT / OK on UART1.");
    Serial.print("UART2 ready on RX=");
    Serial.print(PINS.uart2RxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uart2TxPin);
    Serial.println("Send 1 / 2 on UART2 to switch or reset the TFT screen.");

    servoSetup();
    tftSetup();
}

void loop() {
    uint32_t nowMs = millis();
    static bool wasAtomizerBusy = false;

    pollUart1();
    pollUart2();
    pollUsbScreenCommands();
    servoUpdate(nowMs);
    tftUpdate(nowMs);

    bool isAtomizerBusy = servoIsBusy();
    if (wasAtomizerBusy && !isAtomizerBusy) {
        tftOnAtomizerFinished(nowMs);
    }
    wasAtomizerBusy = isAtomizerBusy;

    if (tftConsumeDetailEntryEvent()) {
        servoStartSequence(nowMs);
        wasAtomizerBusy = true;
    }
}
