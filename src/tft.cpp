#include "tft.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "board_config.h"

namespace {

constexpr int MENU_W = 42;
constexpr int DETAIL_X = MENU_W;
constexpr int DETAIL_W = SCREEN_W - MENU_W;

struct Dish {
    const char *name;
    const char *subtitle;
};

enum class AppView {
    Menu,
    Detail,
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

Adafruit_ST7735 g_tft(PINS.tftCsPin, PINS.tftDcPin, PINS.tftRstPin);
AppView g_currentView = AppView::Menu;
size_t g_currentDishIndex = 0;
Gesture g_lastGesture = Gesture::None;
uint32_t g_lastGestureHandledMs = 0;
String g_serialBuffer;
bool g_detailEntryEvent = false;

void fillPixel(int originX, int originY, int gx, int gy, int size, uint16_t color) {
    g_tft.fillRect(originX + gx * size, originY + gy * size, size - 1, size - 1, color);
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

void fillWhiteEdges() {
    g_tft.fillRect(SCREEN_W - 2, 0, 2, SCREEN_H, ST77XX_WHITE);
    g_tft.fillRect(0, SCREEN_H - 2, SCREEN_W, 2, ST77XX_WHITE);
}

void drawMenuList() {
    g_tft.fillRect(0, 0, MENU_W, SCREEN_H, ST77XX_WHITE);
    g_tft.drawFastVLine(MENU_W - 1, 0, SCREEN_H, ST77XX_BLACK);
    g_tft.setTextSize(1);

    for (size_t i = 0; i < DISH_COUNT; ++i) {
        int y = 10 + static_cast<int>(i) * 24;
        uint16_t itemBg = (i == g_currentDishIndex) ? ST77XX_BLACK : ST77XX_WHITE;
        uint16_t itemFg = (i == g_currentDishIndex) ? ST77XX_WHITE : ST77XX_BLACK;

        g_tft.fillRoundRect(3, y, MENU_W - 6, 18, 3, itemBg);
        g_tft.setCursor(6, y + 5);
        g_tft.setTextColor(itemFg);
        g_tft.print(DISHES[i].name);
    }
}

void drawDetailFrame() {
    g_tft.fillRect(DETAIL_X, 0, DETAIL_W, SCREEN_H, ST77XX_WHITE);
    g_tft.drawRect(DETAIL_X + 3, 3, DETAIL_W - 6, SCREEN_H - 6, ST77XX_BLACK);
}

void renderMenuView() {
    drawMenuList();
    drawDetailFrame();

    g_tft.setTextSize(1);
    g_tft.setTextColor(ST77XX_BLACK);
    g_tft.setCursor(DETAIL_X + 8, 10);
    g_tft.print(DISHES[g_currentDishIndex].name);

    g_tft.setCursor(DETAIL_X + 8, 118);
    g_tft.print(DISHES[g_currentDishIndex].subtitle);

    g_tft.setCursor(DETAIL_X + 8, 132);
    g_tft.print("R up  P down");
    g_tft.setCursor(DETAIL_X + 8, 146);
    g_tft.print("S open");

    drawDishPixelArt(g_currentDishIndex, DETAIL_X + 18, 30, 6);
    fillWhiteEdges();
}

void renderDetailView() {
    g_tft.fillScreen(ST77XX_WHITE);
    g_tft.setTextColor(ST77XX_BLACK);
    g_tft.setTextSize(2);
    g_tft.setCursor(8, 10);
    g_tft.print(DISHES[g_currentDishIndex].name);

    drawDishPixelArt(g_currentDishIndex, 16, 44, 10);

    g_tft.setTextSize(1);
    g_tft.setCursor(8, 136);
    g_tft.print(DISHES[g_currentDishIndex].subtitle);
    g_tft.setCursor(8, 148);
    g_tft.print("Scissors back");
    fillWhiteEdges();
}

void renderCurrentView() {
    g_tft.setRotation(0);
    if (g_currentView == AppView::Menu) {
        renderMenuView();
    } else {
        renderDetailView();
    }
}

void applyGestureToUi(Gesture gesture) {
    if (gesture == Gesture::None) {
        return;
    }

    g_lastGesture = gesture;
    Serial.print("Gesture detected: ");
    Serial.println(gestureToString(gesture));

    if (g_currentView == AppView::Menu) {
        if (gesture == Gesture::Rock) {
            g_currentDishIndex = (g_currentDishIndex + DISH_COUNT - 1) % DISH_COUNT;
        } else if (gesture == Gesture::Paper) {
            g_currentDishIndex = (g_currentDishIndex + 1) % DISH_COUNT;
        } else if (gesture == Gesture::Scissors) {
            g_currentView = AppView::Detail;
            g_detailEntryEvent = true;
        }
    } else if (gesture == Gesture::Scissors) {
        g_currentView = AppView::Menu;
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
            Gesture gesture = parseGestureLabel(g_serialBuffer);
            g_serialBuffer = "";
            return gesture;
        }

        g_serialBuffer += ch;
    }

    return Gesture::None;
}

}  // namespace

void tftSetup() {
    SPI.begin(PINS.tftSclkPin, PINS.tftMisoPin, PINS.tftMosiPin, PINS.tftCsPin);
    g_tft.initR(INITR_BLACKTAB);
    renderCurrentView();
}

void tftUpdate(uint32_t nowMs) {
    Gesture gesture = pollGestureFromSerial();
    if (gesture == Gesture::None) {
        return;
    }
    if (nowMs - g_lastGestureHandledMs < GESTURE_COOLDOWN_MS) {
        return;
    }

    g_lastGestureHandledMs = nowMs;
    applyGestureToUi(gesture);
}

void tftOnGestureRecognized(const String &label) {
    Gesture gesture = parseGestureLabel(label);
    uint32_t nowMs = millis();

    if (gesture == Gesture::None) {
        return;
    }
    if (nowMs - g_lastGestureHandledMs < GESTURE_COOLDOWN_MS) {
        return;
    }

    g_lastGestureHandledMs = nowMs;
    applyGestureToUi(gesture);
}

bool tftConsumeDetailEntryEvent() {
    bool shouldStartServo = g_detailEntryEvent;
    g_detailEntryEvent = false;
    return shouldStartServo;
}
