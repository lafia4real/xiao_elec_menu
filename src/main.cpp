#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include <SPI.h>

// Board selection:
// Keep this set to BOARD_XIAO_ESP32S3_SENSE for the current board.
// Switch to BOARD_XIAO_ESP32S3_PLUS later and only update the pin map below.
#define BOARD_XIAO_ESP32S3_SENSE 1
#define BOARD_XIAO_ESP32S3_PLUS  2
#define TARGET_BOARD BOARD_XIAO_ESP32S3_SENSE

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
    1,   // servoPin
    43,  // tftCsPin
    44,  // tftDcPin
    -1,  // tftRstPin
    7,   // tftSclkPin
    8,   // tftMisoPin
    9,   // tftMosiPin
    44,  // uartRxPin
    43,  // uartTxPin
    "XIAO ESP32S3 Sense"
};
#elif TARGET_BOARD == BOARD_XIAO_ESP32S3_PLUS
constexpr BoardPins PINS = {
    1,   // servoPin
    43,  // tftCsPin
    44,  // tftDcPin
    -1,  // tftRstPin
    7,   // tftSclkPin
    8,   // tftMisoPin
    9,   // tftMosiPin
    44,  // uartRxPin
    43,  // uartTxPin
    "XIAO ESP32S3 Plus"
};
#else
#error "Unsupported TARGET_BOARD value"
#endif

constexpr int CENTER_ANGLE = 90;
constexpr int LEFT_ANGLE = 60;
constexpr int RIGHT_ANGLE = 120;
constexpr uint32_t SERVO_STEP_INTERVAL_MS = 15;
constexpr uint32_t HOLD_LEFT_MS = 800;
constexpr uint32_t HOLD_RIGHT_MS = 800;
constexpr uint32_t HOLD_CENTER_MS = 1000;
constexpr uint32_t GESTURE_COOLDOWN_MS = 700;

constexpr int SCREEN_W = 128;
constexpr int SCREEN_H = 160;
constexpr int MENU_W = 42;
constexpr int DETAIL_X = MENU_W;
constexpr int DETAIL_W = SCREEN_W - MENU_W;

Servo myServo;
Adafruit_ST7735 tft(PINS.tftCsPin, PINS.tftDcPin, PINS.tftRstPin);

enum class ServoPhase {
    Idle,
    MoveToLeft,
    HoldLeft,
    MoveToRight,
    HoldRight,
    MoveToCenter,
    HoldCenter,
};

enum class Gesture {
    None,
    Rock,
    Paper,
    Scissors,
};

enum class AppView {
    Menu,
    Detail,
};

struct Dish {
    const char *name;
    const char *subtitle;
};

const Dish DISHES[] = {
    {"Baozi", "steamed"},
    {"Jiaozi", "dumpling"},
    {"Noodles", "beef"},
    {"Mapo", "tofu"},
    {"Burger", "beef"},
    {"Pizza", "cheese"}
};
constexpr size_t DISH_COUNT = sizeof(DISHES) / sizeof(DISHES[0]);

ServoPhase servoPhase = ServoPhase::Idle;
int currentServoAngle = CENTER_ANGLE;
uint32_t lastServoStepMs = 0;
uint32_t servoPhaseStartMs = 0;

AppView currentView = AppView::Menu;
size_t currentDishIndex = 0;
Gesture lastGesture = Gesture::None;
uint32_t lastGestureHandledMs = 0;
String serialBuffer;

void fillPixel(int originX, int originY, int gx, int gy, int size, uint16_t color) {
    tft.fillRect(originX + gx * size, originY + gy * size, size - 1, size - 1, color);
}

void drawPixelArtBaozi(int originX, int originY, int s) {
    fillPixel(originX, originY, 3, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, ST77XX_WHITE);
    fillPixel(originX, originY, 4, 2, s, ST77XX_WHITE);
    fillPixel(originX, originY, 5, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 1, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 3, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 4, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 5, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 6, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 4, s, ST77XX_WHITE);
    fillPixel(originX, originY, 3, 4, s, 0xC618);
    fillPixel(originX, originY, 4, 4, s, 0xC618);
    fillPixel(originX, originY, 5, 4, s, ST77XX_WHITE);
    fillPixel(originX, originY, 2, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 5, s, ST77XX_BLACK);
}

void drawPixelArtJiaozi(int originX, int originY, int s) {
    fillPixel(originX, originY, 2, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 1, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 3, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 4, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 5, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 6, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 4, s, ST77XX_WHITE);
    fillPixel(originX, originY, 3, 4, s, 0xC618);
    fillPixel(originX, originY, 4, 4, s, 0xC618);
    fillPixel(originX, originY, 5, 4, s, ST77XX_WHITE);
    fillPixel(originX, originY, 3, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 5, s, ST77XX_BLACK);
}

void drawPixelArtNoodles(int originX, int originY, int s) {
    fillPixel(originX, originY, 2, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 6, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, 0xFD20);
    fillPixel(originX, originY, 3, 3, s, 0xFD20);
    fillPixel(originX, originY, 4, 3, s, 0xFD20);
    fillPixel(originX, originY, 5, 3, s, 0xFD20);
    fillPixel(originX, originY, 6, 3, s, 0xFD20);
    fillPixel(originX, originY, 2, 4, s, 0xFD20);
    fillPixel(originX, originY, 3, 4, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 4, 4, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 5, 4, s, 0xA145);
    fillPixel(originX, originY, 6, 4, s, 0xFD20);
    fillPixel(originX, originY, 2, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 6, 5, s, ST77XX_BLACK);
}

void drawPixelArtMapo(int originX, int originY, int s) {
    fillPixel(originX, originY, 2, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 6, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 3, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 4, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 5, 3, s, ST77XX_WHITE);
    fillPixel(originX, originY, 6, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 2, 4, s, ST77XX_RED);
    fillPixel(originX, originY, 3, 4, s, ST77XX_GREEN);
    fillPixel(originX, originY, 4, 4, s, ST77XX_WHITE);
    fillPixel(originX, originY, 5, 4, s, ST77XX_GREEN);
    fillPixel(originX, originY, 6, 4, s, ST77XX_RED);
    fillPixel(originX, originY, 2, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 5, s, ST77XX_BLACK);
    fillPixel(originX, originY, 6, 5, s, ST77XX_BLACK);
}

void drawPixelArtBurger(int originX, int originY, int s) {
    fillPixel(originX, originY, 3, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, 0xD34A);
    fillPixel(originX, originY, 4, 2, s, 0xD34A);
    fillPixel(originX, originY, 5, 2, s, 0xD34A);
    fillPixel(originX, originY, 6, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 3, s, 0xD34A);
    fillPixel(originX, originY, 4, 3, s, 0xD34A);
    fillPixel(originX, originY, 5, 3, s, 0xD34A);
    fillPixel(originX, originY, 6, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 4, s, 0x07E0);
    fillPixel(originX, originY, 3, 4, s, 0x07E0);
    fillPixel(originX, originY, 4, 4, s, 0x07E0);
    fillPixel(originX, originY, 5, 4, s, 0x07E0);
    fillPixel(originX, originY, 6, 4, s, 0x07E0);
    fillPixel(originX, originY, 2, 5, s, 0xA145);
    fillPixel(originX, originY, 3, 5, s, 0xA145);
    fillPixel(originX, originY, 4, 5, s, 0xA145);
    fillPixel(originX, originY, 5, 5, s, 0xA145);
    fillPixel(originX, originY, 6, 5, s, 0xA145);
    fillPixel(originX, originY, 2, 6, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 3, 6, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 4, 6, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 5, 6, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 6, 6, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 2, 7, s, 0xD34A);
    fillPixel(originX, originY, 3, 7, s, 0xD34A);
    fillPixel(originX, originY, 4, 7, s, 0xD34A);
    fillPixel(originX, originY, 5, 7, s, 0xD34A);
    fillPixel(originX, originY, 6, 7, s, 0xD34A);
}

void drawPixelArtPizza(int originX, int originY, int s) {
    fillPixel(originX, originY, 4, 1, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 2, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 5, 2, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 3, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 4, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 5, 3, s, ST77XX_RED);
    fillPixel(originX, originY, 6, 3, s, ST77XX_BLACK);
    fillPixel(originX, originY, 1, 4, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 4, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 3, 4, s, ST77XX_RED);
    fillPixel(originX, originY, 4, 4, s, ST77XX_RED);
    fillPixel(originX, originY, 5, 4, s, ST77XX_RED);
    fillPixel(originX, originY, 6, 4, s, ST77XX_YELLOW);
    fillPixel(originX, originY, 7, 4, s, ST77XX_BLACK);
    fillPixel(originX, originY, 2, 5, s, 0xD34A);
    fillPixel(originX, originY, 3, 5, s, 0xD34A);
    fillPixel(originX, originY, 4, 5, s, 0xD34A);
    fillPixel(originX, originY, 5, 5, s, 0xD34A);
    fillPixel(originX, originY, 6, 5, s, 0xD34A);
    fillPixel(originX, originY, 3, 6, s, ST77XX_BLACK);
    fillPixel(originX, originY, 4, 6, s, ST77XX_BLACK);
    fillPixel(originX, originY, 5, 6, s, ST77XX_BLACK);
}

void drawDishPixelArt(size_t dishIndex, int originX, int originY, int pixelSize) {
    switch (dishIndex) {
        case 0:
            drawPixelArtBaozi(originX, originY, pixelSize);
            break;
        case 1:
            drawPixelArtJiaozi(originX, originY, pixelSize);
            break;
        case 2:
            drawPixelArtNoodles(originX, originY, pixelSize);
            break;
        case 3:
            drawPixelArtMapo(originX, originY, pixelSize);
            break;
        case 4:
            drawPixelArtBurger(originX, originY, pixelSize);
            break;
        case 5:
            drawPixelArtPizza(originX, originY, pixelSize);
            break;
        default:
            drawPixelArtBaozi(originX, originY, pixelSize);
            break;
    }
}

const char *gestureToString(Gesture gesture) {
    switch (gesture) {
        case Gesture::Rock:
            return "ROCK";
        case Gesture::Paper:
            return "PAPER";
        case Gesture::Scissors:
            return "SCISSORS";
        case Gesture::None:
        default:
            return "NONE";
    }
}

String normalizeLabel(String label) {
    label.trim();
    label.toLowerCase();
    return label;
}

Gesture parseGestureLabel(const String &rawLabel) {
    String label = normalizeLabel(rawLabel);

    if (label == "rock" || label == "stone") {
        return Gesture::Rock;
    }
    if (label == "paper") {
        return Gesture::Paper;
    }
    if (label == "scissors" || label == "scissor") {
        return Gesture::Scissors;
    }

    return Gesture::None;
}

void enterServoPhase(ServoPhase nextPhase, uint32_t nowMs) {
    servoPhase = nextPhase;
    servoPhaseStartMs = nowMs;

    switch (servoPhase) {
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

void startServoSequence(uint32_t nowMs) {
    currentServoAngle = CENTER_ANGLE;
    myServo.write(currentServoAngle);
    lastServoStepMs = nowMs;
    enterServoPhase(ServoPhase::MoveToLeft, nowMs);
}

void updateServo(uint32_t nowMs) {
    if (servoPhase == ServoPhase::Idle) {
        return;
    }
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
                enterServoPhase(ServoPhase::Idle, nowMs);
            }
            break;

        case ServoPhase::Idle:
            break;
    }
}

void drawMenuList() {
    tft.fillRect(0, 0, MENU_W, SCREEN_H, ST77XX_WHITE);
    tft.drawFastVLine(MENU_W - 1, 0, SCREEN_H, ST77XX_BLACK);
    tft.setTextSize(1);

    for (size_t i = 0; i < DISH_COUNT; ++i) {
        int y = 10 + static_cast<int>(i) * 24;
        uint16_t itemBg = (i == currentDishIndex) ? ST77XX_BLACK : ST77XX_WHITE;
        uint16_t itemFg = (i == currentDishIndex) ? ST77XX_WHITE : ST77XX_BLACK;

        tft.fillRoundRect(3, y, MENU_W - 6, 18, 3, itemBg);
        tft.setCursor(6, y + 5);
        tft.setTextColor(itemFg);
        tft.print(DISHES[i].name);
    }
}

void drawDetailFrame() {
    tft.fillRect(DETAIL_X, 0, DETAIL_W, SCREEN_H, ST77XX_WHITE);
    tft.drawRect(DETAIL_X + 3, 3, DETAIL_W - 6, SCREEN_H - 6, ST77XX_BLACK);
}

void renderMenuView() {
    drawMenuList();
    drawDetailFrame();

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(DETAIL_X + 8, 10);
    tft.print(DISHES[currentDishIndex].name);

    tft.setCursor(DETAIL_X + 8, 118);
    tft.print(DISHES[currentDishIndex].subtitle);

    tft.setCursor(DETAIL_X + 8, 132);
    tft.print("R up  P down");
    tft.setCursor(DETAIL_X + 8, 146);
    tft.print("S open");

    drawDishPixelArt(currentDishIndex, DETAIL_X + 18, 30, 6);
}

void renderDetailView() {
    tft.fillScreen(ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setCursor(8, 10);
    tft.print(DISHES[currentDishIndex].name);

    drawDishPixelArt(currentDishIndex, 16, 44, 10);

    tft.setTextSize(1);
    tft.setCursor(8, 136);
    tft.print(DISHES[currentDishIndex].subtitle);
    tft.setCursor(8, 148);
    tft.print("Scissors back");
}

void renderCurrentView() {
    tft.setRotation(0);
    if (currentView == AppView::Menu) {
        renderMenuView();
    } else {
        renderDetailView();
    }
}

void applyGestureToUi(Gesture gesture) {
    if (gesture == Gesture::None) {
        return;
    }

    lastGesture = gesture;
    Serial.print("Gesture detected: ");
    Serial.println(gestureToString(gesture));

    if (currentView == AppView::Menu) {
        if (gesture == Gesture::Rock) {
            currentDishIndex = (currentDishIndex + DISH_COUNT - 1) % DISH_COUNT;
        } else if (gesture == Gesture::Paper) {
            currentDishIndex = (currentDishIndex + 1) % DISH_COUNT;
        } else if (gesture == Gesture::Scissors) {
            currentView = AppView::Detail;
            startServoSequence(millis());
        }
    } else if (gesture == Gesture::Scissors) {
        currentView = AppView::Menu;
    }

    renderCurrentView();
}

Gesture pollGestureFromSerial() {
    while (Serial.available() > 0) {
        char ch = static_cast<char>(Serial.read());

        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            Gesture gesture = parseGestureLabel(serialBuffer);
            serialBuffer = "";
            return gesture;
        }

        serialBuffer += ch;
    }

    return Gesture::None;
}

void updateGestureInput(uint32_t nowMs) {
    Gesture gesture = pollGestureFromSerial();
    if (gesture == Gesture::None) {
        return;
    }
    if (nowMs - lastGestureHandledMs < GESTURE_COOLDOWN_MS) {
        return;
    }

    lastGestureHandledMs = nowMs;
    applyGestureToUi(gesture);
}

void onGestureRecognized(const String &label) {
    Gesture gesture = parseGestureLabel(label);
    uint32_t nowMs = millis();

    if (gesture == Gesture::None) {
        return;
    }
    if (nowMs - lastGestureHandledMs < GESTURE_COOLDOWN_MS) {
        return;
    }

    lastGestureHandledMs = nowMs;
    applyGestureToUi(gesture);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Gesture order menu start");
    Serial.print("Target board: ");
    Serial.println(PINS.boardName);
    Serial.println("Type rock / paper / scissors in Serial Monitor to test.");

    myServo.setPeriodHertz(50);
    myServo.attach(PINS.servoPin, 500, 2400);
    myServo.write(CENTER_ANGLE);
    currentServoAngle = CENTER_ANGLE;

    SPI.begin(PINS.tftSclkPin, PINS.tftMisoPin, PINS.tftMosiPin, PINS.tftCsPin);
    tft.initR(INITR_BLACKTAB);
    renderCurrentView();

    uint32_t nowMs = millis();
    lastServoStepMs = nowMs;
    servoPhaseStartMs = nowMs;
}

void loop() {
    uint32_t nowMs = millis();
    updateServo(nowMs);
    updateGestureInput(nowMs);
}
