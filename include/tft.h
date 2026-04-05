#ifndef TFT_MENU_H
#define TFT_MENU_H

#include <Arduino.h>

enum class Gesture {
    None,
    Rock,
    Paper,
    Scissors,
};

void tftSetup();
void tftUpdate(uint32_t nowMs);
void tftOnGestureRecognized(const String &label);
void tftOnStartSignal(uint32_t nowMs);
void tftOnAtomizerFinished(uint32_t nowMs);
void tftToggleScreen(uint32_t nowMs);
void tftResetToWelcome();
bool tftConsumeDetailEntryEvent();

#endif
