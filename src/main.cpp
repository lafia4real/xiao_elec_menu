#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include <SPI.h>

// Wiring summary for XIAO ESP32S3 Sense
// Servo signal -> GPIO1
//
// TFT 1.8 inch 128x160 SPI (common ST7735/ST7735S)
// TFT VCC  -> 3V3
// TFT GND  -> GND
// TFT SCK  -> GPIO7   (D8)
// TFT MISO -> GPIO8   (D9, optional)
// TFT MOSI -> GPIO9   (D10)
// TFT CS   -> GPIO43  (D6)
// TFT DC   -> GPIO44  (D7)
// TFT RST  -> XIAO RST pin or 3V3
//
// Note:
// XIAO ESP32S3 does not expose a convenient GPIO12 on the normal header pins.
// For the TFT reset line, the simplest stable approach is to hard-wire the TFT
// RST pin to the board RST pin (preferred) or 3V3, and let the library skip
// software reset control by using -1 here.

constexpr int SERVO_PIN = 1;
constexpr int TFT_CS_PIN = 43;
constexpr int TFT_DC_PIN = 44;
constexpr int TFT_RST_PIN = -1;
constexpr int TFT_SCLK_PIN = 7;
constexpr int TFT_MISO_PIN = 8;
constexpr int TFT_MOSI_PIN = 9;

constexpr int CENTER_ANGLE = 90;
constexpr int LEFT_ANGLE = 60;
constexpr int RIGHT_ANGLE = 120;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 15;
constexpr uint32_t HOLD_LEFT_MS = 800;
constexpr uint32_t HOLD_RIGHT_MS = 800;
constexpr uint32_t HOLD_CENTER_MS = 1000;

Servo myServo;
Adafruit_ST7735 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

enum class ServoPhase {
    MoveToLeft,
    HoldLeft,
    MoveToRight,
    HoldRight,
    MoveToCenter,
    HoldCenter,
};

enum class TftPage {
    Startup,
    ColorBars,
    Geometry,
    Text,
    Rotation0,
    Rotation1,
    Rotation2,
    Rotation3,
    Final,
};

ServoPhase servoPhase = ServoPhase::MoveToLeft;
TftPage currentTftPage = TftPage::Startup;
int currentServoAngle = CENTER_ANGLE;
uint32_t lastServoStepMs = 0;
uint32_t servoPhaseStartMs = 0;
uint32_t tftPageStartMs = 0;

void centerText(const String &text, int y, uint16_t color, uint8_t textSize) {
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    tft.setTextSize(textSize);
    tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int16_t x = (tft.width() - w) / 2;

    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.print(text);
}

void enterServoPhase(ServoPhase nextPhase, uint32_t nowMs) {
    servoPhase = nextPhase;
    servoPhaseStartMs = nowMs;

    switch (servoPhase) {
        case ServoPhase::MoveToLeft:
            Serial.println("Move to 60 deg");
            break;
        case ServoPhase::HoldLeft:
            break;
        case ServoPhase::MoveToRight:
            Serial.println("Move to 120 deg");
            break;
        case ServoPhase::HoldRight:
            break;
        case ServoPhase::MoveToCenter:
            Serial.println("Back to center");
            break;
        case ServoPhase::HoldCenter:
            break;
    }
}

void updateServo(uint32_t nowMs) {
    if (nowMs - lastServoStepMs < SERVO_STEP_INTERVAL_MS) {
        return;
    }

    lastServoStepMs = nowMs;

    switch (servoPhase) {
        case ServoPhase::MoveToLeft:
            if (currentServoAngle > LEFT_ANGLE) {
                currentServoAngle--;
                myServo.write(currentServoAngle);
            } else {
                enterServoPhase(ServoPhase::HoldLeft, nowMs);
            }
            break;

        case ServoPhase::HoldLeft:
            if (nowMs - servoPhaseStartMs >= HOLD_LEFT_MS) {
                enterServoPhase(ServoPhase::MoveToRight, nowMs);
            }
            break;

        case ServoPhase::MoveToRight:
            if (currentServoAngle < RIGHT_ANGLE) {
                currentServoAngle++;
                myServo.write(currentServoAngle);
            } else {
                enterServoPhase(ServoPhase::HoldRight, nowMs);
            }
            break;

        case ServoPhase::HoldRight:
            if (nowMs - servoPhaseStartMs >= HOLD_RIGHT_MS) {
                enterServoPhase(ServoPhase::MoveToCenter, nowMs);
            }
            break;

        case ServoPhase::MoveToCenter:
            if (currentServoAngle > CENTER_ANGLE) {
                currentServoAngle--;
                myServo.write(currentServoAngle);
            } else if (currentServoAngle < CENTER_ANGLE) {
                currentServoAngle++;
                myServo.write(currentServoAngle);
            } else {
                enterServoPhase(ServoPhase::HoldCenter, nowMs);
            }
            break;

        case ServoPhase::HoldCenter:
            if (nowMs - servoPhaseStartMs >= HOLD_CENTER_MS) {
                enterServoPhase(ServoPhase::MoveToLeft, nowMs);
            }
            break;
    }
}

void renderStartupScreen() {
    tft.setRotation(0);
    tft.fillScreen(ST77XX_BLACK);
    centerText("XIAO ESP32S3", 16, ST77XX_WHITE, 2);
    centerText("SERVO + TFT", 46, ST77XX_YELLOW, 2);
    centerText("128 x 160", 80, ST77XX_CYAN, 2);
    centerText("ST7735", 112, ST77XX_GREEN, 2);
}

void renderColorBars() {
    tft.setRotation(0);
    const uint16_t colors[] = {
        ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE,
        ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_YELLOW,
        ST77XX_WHITE, ST77XX_BLACK
    };

    tft.fillScreen(ST77XX_BLACK);
    int16_t barHeight = tft.height() / 8;
    for (int i = 0; i < 8; ++i) {
        tft.fillRect(0, i * barHeight, tft.width(), barHeight, colors[i]);
    }
    tft.drawRect(0, 0, tft.width(), tft.height(), ST77XX_WHITE);
}

void renderGeometryTest() {
    tft.setRotation(0);
    tft.fillScreen(ST77XX_BLACK);

    for (int16_t i = 0; i < tft.width(); i += 12) {
        tft.drawLine(0, 0, i, tft.height() - 1, ST77XX_RED);
        tft.drawLine(tft.width() - 1, tft.height() - 1, i, 0, ST77XX_BLUE);
    }

    for (int16_t i = 0; i < tft.height(); i += 12) {
        tft.drawLine(0, tft.height() - 1, tft.width() - 1, i, ST77XX_GREEN);
        tft.drawLine(tft.width() - 1, 0, 0, i, ST77XX_YELLOW);
    }

    tft.fillCircle(tft.width() / 2, tft.height() / 2, 18, ST77XX_MAGENTA);
    tft.drawCircle(tft.width() / 2, tft.height() / 2, 34, ST77XX_WHITE);
}

void renderTextTest() {
    tft.setRotation(0);
    tft.fillScreen(ST77XX_BLUE);
    tft.setTextWrap(false);

    tft.setCursor(8, 10);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.println("Text test:");

    tft.setCursor(8, 30);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.println("Hello");

    tft.setCursor(8, 60);
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(2);
    tft.println("TFT LCD");

    tft.setCursor(8, 95);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.println("Servo still running");

    tft.setCursor(8, 115);
    tft.setTextColor(ST77XX_MAGENTA);
    tft.println("Rotation test next");
}

void renderRotationPage(uint8_t rotation) {
    tft.setRotation(rotation);
    tft.fillScreen(ST77XX_BLACK);
    tft.drawRect(0, 0, tft.width(), tft.height(), ST77XX_WHITE);
    centerText("Rotation", 22, ST77XX_YELLOW, 2);
    centerText(String(rotation), 64, ST77XX_GREEN, 4);
    centerText(String(tft.width()) + "x" + String(tft.height()), 120, ST77XX_CYAN, 1);
}

void renderFinalScreen() {
    tft.setRotation(0);
    tft.fillScreen(ST77XX_BLACK);
    centerText("Display OK", 24, ST77XX_GREEN, 2);
    centerText("Servo on GPIO1", 62, ST77XX_WHITE, 1);
    centerText("GPIO2~6 reserved", 78, ST77XX_WHITE, 1);
    centerText("TFT on 7/8/9/43/44", 94, ST77XX_CYAN, 1);
}

void renderTftPage(TftPage page) {
    switch (page) {
        case TftPage::Startup:
            renderStartupScreen();
            break;
        case TftPage::ColorBars:
            renderColorBars();
            break;
        case TftPage::Geometry:
            renderGeometryTest();
            break;
        case TftPage::Text:
            renderTextTest();
            break;
        case TftPage::Rotation0:
            renderRotationPage(0);
            break;
        case TftPage::Rotation1:
            renderRotationPage(1);
            break;
        case TftPage::Rotation2:
            renderRotationPage(2);
            break;
        case TftPage::Rotation3:
            renderRotationPage(3);
            break;
        case TftPage::Final:
            renderFinalScreen();
            break;
    }
}

uint32_t pageDurationMs(TftPage page) {
    switch (page) {
        case TftPage::Startup:
            return 1800;
        case TftPage::ColorBars:
            return 1600;
        case TftPage::Geometry:
            return 1800;
        case TftPage::Text:
            return 1800;
        case TftPage::Rotation0:
        case TftPage::Rotation1:
        case TftPage::Rotation2:
        case TftPage::Rotation3:
            return 1200;
        case TftPage::Final:
            return 2500;
    }

    return 1500;
}

TftPage nextPage(TftPage page) {
    switch (page) {
        case TftPage::Startup:
            return TftPage::ColorBars;
        case TftPage::ColorBars:
            return TftPage::Geometry;
        case TftPage::Geometry:
            return TftPage::Text;
        case TftPage::Text:
            return TftPage::Rotation0;
        case TftPage::Rotation0:
            return TftPage::Rotation1;
        case TftPage::Rotation1:
            return TftPage::Rotation2;
        case TftPage::Rotation2:
            return TftPage::Rotation3;
        case TftPage::Rotation3:
            return TftPage::Final;
        case TftPage::Final:
            return TftPage::ColorBars;
    }

    return TftPage::ColorBars;
}

void updateTft(uint32_t nowMs) {
    if (nowMs - tftPageStartMs < pageDurationMs(currentTftPage)) {
        return;
    }

    currentTftPage = nextPage(currentTftPage);
    tftPageStartMs = nowMs;
    renderTftPage(currentTftPage);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Servo + TFT test start");
    Serial.println("TFT RST is hard-wired, software reset pin disabled");

    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    myServo.write(CENTER_ANGLE);
    currentServoAngle = CENTER_ANGLE;

    SPI.begin(TFT_SCLK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);

    // Most 1.8 inch 128x160 modules use INITR_BLACKTAB.
    // If you still get a white screen or wrong colors, try INITR_GREENTAB
    // and then INITR_REDTAB.
    tft.initR(INITR_BLACKTAB);
    renderTftPage(currentTftPage);

    uint32_t nowMs = millis();
    lastServoStepMs = nowMs;
    servoPhaseStartMs = nowMs;
    tftPageStartMs = nowMs;

    enterServoPhase(ServoPhase::MoveToLeft, nowMs);
}

void loop() {
    uint32_t nowMs = millis();
    updateServo(nowMs);
    updateTft(nowMs);
}
