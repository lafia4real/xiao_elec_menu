#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>

// Board selection:
// Switch TARGET_BOARD to match the active hardware and keep the rest of the
// app logic unchanged.
#define BOARD_XIAO_ESP32S3_SENSE 1
#define BOARD_XIAO_ESP32S3_PLUS  2
#define BOARD_JLC_ESP32S3R8N8    3
#define TARGET_BOARD BOARD_JLC_ESP32S3R8N8

struct BoardPins {
    int servoPin;
    int tftCsPin;
    int tftDcPin;
    int tftRstPin;
    int tftSclkPin;
    int tftMisoPin;
    int tftMosiPin;
    int uartRxPin;
    int uartTxPin;
    const char *boardName;
};

#if TARGET_BOARD == BOARD_XIAO_ESP32S3_SENSE
constexpr BoardPins PINS = {
    1,
    43,
    44,
    8,
    7,
    8,
    9,
    44,
    43,
    "XIAO ESP32S3 Sense"
};
#elif TARGET_BOARD == BOARD_XIAO_ESP32S3_PLUS
constexpr BoardPins PINS = {
    1,
    3,
    4,
    8,
    7,
    8,
    9,
    44,
    43,
    "XIAO ESP32S3 Plus"
};
#elif TARGET_BOARD == BOARD_JLC_ESP32S3R8N8
constexpr BoardPins PINS = {
    1,
    3,
    4,
    8,
    7,
    8,
    9,
    46,
    45,
    "JLC ESP32S3R8N8 Dev Board"
};
#else
#error "Unsupported TARGET_BOARD value"
#endif

constexpr int SCREEN_W = 128;
constexpr int SCREEN_H = 160;

constexpr int CENTER_ANGLE = 90;
constexpr int LEFT_ANGLE = 60;
constexpr int RIGHT_ANGLE = 120;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 15;
constexpr uint32_t HOLD_LEFT_MS = 800;
constexpr uint32_t HOLD_RIGHT_MS = 800;
constexpr uint32_t HOLD_CENTER_MS = 1000;

constexpr uint32_t GESTURE_COOLDOWN_MS = 700;

#endif
