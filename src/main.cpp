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

bool isExternalScreenCommand(const String &cmd) {
    return cmd == "1" || cmd == "2" || cmd == "3" ||
           cmd == "11" || cmd == "22" || cmd == "33" ||
           cmd == "111";
}

void sendScreenCommand(const String &rawCmd) {
    String cmd = normalizeCommand(rawCmd);

    if (cmd.length() == 0) {
        return;
    }

    if (isExternalScreenCommand(cmd)) {
        Serial.print("[ACTION] SCREEN -> send to external display: ");
        Serial.println(cmd);
        Serial2.print(cmd);
        Serial2.print('\n');
    } else {
        Serial.print("[WARN] Unknown screen command: ");
        Serial.println(cmd);
    }
}

void syncExternalScreenState() {
    static String lastSentScreen;
    String desiredScreen;

    if (tftIsWelcomeView()) {
        desiredScreen = "111";
    } else {
        uint8_t slot = tftGetExternalMenuSlot();
        if (tftIsDetailView()) {
            desiredScreen = String(slot) + String(slot);
        } else {
            desiredScreen = String(slot);
        }
    }

    if (desiredScreen == lastSentScreen) {
        return;
    }

    sendScreenCommand(desiredScreen);
    lastSentScreen = desiredScreen;
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

    if (isExternalScreenCommand(cmd)) {
        Serial.print("[USB SCREEN RECV] ");
        Serial.println(cmd);
        sendScreenCommand(cmd);
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
    Serial.println("Type 1/2/3/11/22/33/111 in Serial Monitor to test the external display.");
    Serial.print("UART1 ready on RX=");
    Serial.print(PINS.uartRxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uartTxPin);
    Serial.println("Send START / LEFT / RIGHT / OK on UART1.");
    Serial.print("UART2 ready on RX=");
    Serial.print(PINS.uart2RxPin);
    Serial.print(", TX=");
    Serial.println(PINS.uart2TxPin);
    Serial.println("Send 1/2/3/11/22/33/111 on UART2 for the external display.");

    servoSetup();
    tftSetup();
    syncExternalScreenState();
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

    syncExternalScreenState();
}
