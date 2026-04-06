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

bool isScreenControlCommand(const String &cmd) {
    return cmd == "HELLO" || cmd == "LEFT" || cmd == "RIGHT" || cmd == "OK" || cmd == "BACK";
}

void sendScreenCommand(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);

    if (cmd.length() == 0) {
        return;
    }

    if (isScreenControlCommand(cmd)) {
        Serial.print("[ACTION] SCREEN -> send to external display: ");
        Serial.println(cmd);
        Serial2.print(cmd);
        Serial2.print('\n');
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

    if (cmd == "HELLO") {
        if (tftIsWelcomeView()) {
            Serial.println("[ACTION] HELLO -> enter menu");
            tftOnHelloSignal(nowMs);
            sendScreenCommand("HELLO");
        } else {
            Serial.println("[ACTION] HELLO ignored outside init view");
        }
    } else if (cmd == "LEFT") {
        if (!tftIsWelcomeView() && !tftIsDetailView()) {
            Serial.println("[ACTION] LEFT -> menu move left");
            dispatchGestureLabel("rock");
            sendScreenCommand("LEFT");
        } else {
            Serial.println("[ACTION] LEFT ignored outside menu view");
        }
    } else if (cmd == "RIGHT") {
        if (!tftIsWelcomeView() && !tftIsDetailView()) {
            Serial.println("[ACTION] RIGHT -> menu move right");
            dispatchGestureLabel("paper");
            sendScreenCommand("RIGHT");
        } else {
            Serial.println("[ACTION] RIGHT ignored outside menu view");
        }
    } else if (cmd == "OK") {
        if (tftIsDetailView()) {
            Serial.println("[ACTION] OK -> detail heartbeat");
            tftOnOkSignal(nowMs);
            sendScreenCommand("OK");
        } else if (!tftIsWelcomeView()) {
            Serial.println("[ACTION] OK -> open detail");
            dispatchGestureLabel("scissors");
            tftOnOkSignal(nowMs);
            sendScreenCommand("OK");
        } else {
            Serial.println("[ACTION] OK ignored outside menu view");
        }
    } else if (cmd == "BACK") {
        Serial.println("[ACTION] BACK -> enter back flow");
        tftOnBackSignal(nowMs);
        sendScreenCommand("BACK");
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

void pollUsbCommands() {
    if (!Serial.available()) {
        return;
    }

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.length() == 0) {
        Serial.println("[USB RECV] <empty>");
        return;
    }

    Serial.print("[USB RAW] ");
    Serial.println(cmd);

    if (cmd == "HELLO" || cmd == "LEFT" || cmd == "RIGHT" || cmd == "OK" || cmd == "BACK") {
        handleUartCommand(cmd);
    } else if (cmd == "ROCK" || cmd == "PAPER" || cmd == "SCISSORS") {
        dispatchGestureLabel(cmd);
    } else {
        Serial.print("[USB WARN] Unknown command: ");
        Serial.println(cmd);
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
    Serial.println("USB Serial supports HELLO / LEFT / RIGHT / OK / BACK.");
    Serial.println("USB Serial can also forward HELLO / LEFT / RIGHT / OK / BACK to the external display.");
    Serial.print("UART1 ready on RX=");
    Serial.print(PINS.uartRxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uartTxPin);
    Serial.println("Send HELLO / LEFT / RIGHT / OK on UART1.");
    Serial.print("UART2 ready on RX=");
    Serial.print(PINS.uart2RxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uart2TxPin);
    Serial.println("Send HELLO / LEFT / RIGHT / OK / BACK on UART2 for the external display.");

    servoSetup();
    tftSetup();
    tftResetToWelcome();
    Serial.println("UI reset to init screen on boot");
}

void loop() {
    uint32_t nowMs = millis();
    static bool wasAtomizerBusy = false;

    pollUart1();
    pollUart2();
    pollUsbCommands();
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

    if (tftConsumeBackEvent()) {
        Serial.println("[ACTION] BACK -> send to external display");
        sendScreenCommand("BACK");
    }
}
