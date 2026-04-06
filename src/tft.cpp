#include "tft.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "board_config.h"

namespace {

constexpr int MENU_W = 42;
constexpr int DETAIL_X = MENU_W;
constexpr int DETAIL_W = SCREEN_W - MENU_W;
constexpr uint32_t DETAIL_OK_TIMEOUT_MS = 2000;
struct Dish {
    const char *name;
    const char *subtitle;
};

enum class AppView {
    Welcome,
    Menu,
    Detail,
};

const Dish DISHES[] = {
    {"Baozi", "steamed"},
    {"Jiaozi", "dumpling"},
    {"Noodles", "beef"}
};
constexpr size_t DISH_COUNT = sizeof(DISHES) / sizeof(DISHES[0]);

constexpr uint8_t LETTER_W[5] = {
    0b10001,
    0b10001,
    0b10101,
    0b10101,
    0b01010
};

constexpr uint8_t LETTER_E[5] = {
    0b11111,
    0b10000,
    0b11110,
    0b10000,
    0b11111
};

constexpr uint8_t LETTER_L[5] = {
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b11111
};

constexpr uint8_t LETTER_C[5] = {
    0b01111,
    0b10000,
    0b10000,
    0b10000,
    0b01111
};

constexpr uint8_t LETTER_O[5] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

constexpr uint8_t LETTER_M[5] = {
    0b10001,
    0b11011,
    0b10101,
    0b10001,
    0b10001
};

constexpr const uint8_t *WELCOME_WORD[] = {
    LETTER_W,
    LETTER_E,
    LETTER_L,
    LETTER_C,
    LETTER_O,
    LETTER_M,
    LETTER_E
};
constexpr size_t WELCOME_WORD_LEN = sizeof(WELCOME_WORD) / sizeof(WELCOME_WORD[0]);

Adafruit_ST7735 g_tft(PINS.tftCsPin, PINS.tftDcPin, PINS.tftRstPin);
AppView g_currentView = AppView::Welcome;
size_t g_currentDishIndex = 0;
Gesture g_lastGesture = Gesture::None;
uint32_t g_lastGestureHandledMs = 0;
uint32_t g_lastOkSignalMs = 0;
bool g_detailExitArmed = false;
bool g_backEvent = false;
bool g_detailEntryEvent = false;

void fillPixel(int originX, int originY, int gx, int gy, int size, uint16_t color) {
    g_tft.fillRect(originX + gx * size, originY + gy * size, size - 1, size - 1, color);
}

void drawGlyph5x5(const uint8_t glyph[5], int originX, int originY, int pixelSize, uint16_t color) {
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[row] & (1 << (4 - col))) {
                fillPixel(originX, originY, col, row, pixelSize, color);
            }
        }
    }
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

void renderWelcomeView() {
    g_tft.fillScreen(ST77XX_WHITE);

    const int pixelSize = 3;
    const int glyphSpacing = 2;
    const int totalWidth = static_cast<int>(WELCOME_WORD_LEN) * 5 * pixelSize +
                           static_cast<int>(WELCOME_WORD_LEN - 1) * glyphSpacing;
    int x = (SCREEN_W - totalWidth) / 2;
    const int y = 24;

    for (size_t i = 0; i < WELCOME_WORD_LEN; ++i) {
        drawGlyph5x5(WELCOME_WORD[i], x, y, pixelSize, ST77XX_BLACK);
        x += 5 * pixelSize + glyphSpacing;
    }

    g_tft.fillRoundRect(14, 78, 100, 28, 6, ST77XX_BLACK);
    g_tft.setTextColor(ST77XX_WHITE);
    g_tft.setTextSize(2);
    g_tft.setCursor(22, 86);
    g_tft.print("HELLO");

    g_tft.setTextColor(ST77XX_BLACK);
    g_tft.setTextSize(1);
    g_tft.setCursor(19, 122);
    g_tft.print("Send HELLO on UART1");
    g_tft.setCursor(18, 138);
    g_tft.print("to open the menu");
    fillWhiteEdges();
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
    g_tft.print("L up R down");
    g_tft.setCursor(DETAIL_X + 8, 146);
    g_tft.print("OK open");

    drawDishPixelArt(g_currentDishIndex, DETAIL_X + 18, 30, 6);
    fillWhiteEdges();
}

void renderDetailView() {
    g_tft.fillScreen(ST77XX_WHITE);
    g_tft.setTextColor(ST77XX_BLACK);
    g_tft.setTextSize(2);
    g_tft.setCursor(8, 10);
    g_tft.print(DISHES[g_currentDishIndex].name);

    drawDishPixelArt(g_currentDishIndex, 16, 40, 10);

    g_tft.setTextSize(1);
    g_tft.setCursor(8, 128);
    g_tft.print(DISHES[g_currentDishIndex].subtitle);
    g_tft.setCursor(8, 142);
    g_tft.print("Atomizer running");
    g_tft.setCursor(8, 152);
    g_tft.print("Return after finish");
    fillWhiteEdges();
}

void renderCurrentView() {
    g_tft.setRotation(0);
    if (g_currentView == AppView::Welcome) {
        renderWelcomeView();
    } else if (g_currentView == AppView::Menu) {
        renderMenuView();
    } else {
        renderDetailView();
    }
}

void setView(AppView view) {
    if (g_currentView == view) {
        return;
    }

    g_currentView = view;
    renderCurrentView();
}

void applyGestureToUi(Gesture gesture) {
    if (gesture == Gesture::None || g_currentView != AppView::Menu) {
        return;
    }

    g_lastGesture = gesture;
    Serial.print("Gesture detected: ");
    Serial.println(gestureToString(gesture));

    if (gesture == Gesture::Rock) {
        g_currentDishIndex = (g_currentDishIndex + DISH_COUNT - 1) % DISH_COUNT;
    } else if (gesture == Gesture::Paper) {
        g_currentDishIndex = (g_currentDishIndex + 1) % DISH_COUNT;
    } else if (gesture == Gesture::Scissors) {
        g_currentView = AppView::Detail;
        g_lastOkSignalMs = 0;
        g_detailExitArmed = false;
        Serial.println("Detail opened, waiting for atomizer to finish");
        g_detailEntryEvent = true;
    }

    renderCurrentView();
}

}  // namespace

void tftSetup() {
    SPI.begin(PINS.tftSclkPin, PINS.tftMisoPin, PINS.tftMosiPin, PINS.tftCsPin);
    g_tft.initR(INITR_BLACKTAB);
    renderCurrentView();
}

void tftUpdate(uint32_t nowMs) {
    if (g_currentView == AppView::Detail &&
        g_detailExitArmed &&
        g_lastOkSignalMs != 0 &&
        nowMs - g_lastOkSignalMs >= DETAIL_OK_TIMEOUT_MS) {
        g_lastGestureHandledMs = nowMs;
        g_lastOkSignalMs = 0;
        g_detailExitArmed = false;
        g_backEvent = true;
        Serial.println("OK timeout after atomizer finish, back to current menu");
        setView(AppView::Menu);
    }
}

void tftOnGestureRecognized(const String &label) {
    Gesture gesture = parseGestureLabel(label);
    uint32_t nowMs = millis();

    if (gesture == Gesture::None || g_currentView != AppView::Menu) {
        return;
    }
    if (nowMs - g_lastGestureHandledMs < GESTURE_COOLDOWN_MS) {
        return;
    }

    g_lastGestureHandledMs = nowMs;
    applyGestureToUi(gesture);
}

void tftOnHelloSignal(uint32_t nowMs) {
    if (g_currentView == AppView::Welcome) {
        g_currentDishIndex = 0;
        g_lastGestureHandledMs = nowMs;
        g_lastOkSignalMs = 0;
        g_detailExitArmed = false;
        Serial.println("HELLO received, entering menu 1");
        setView(AppView::Menu);
    }
}

void tftOnOkSignal(uint32_t nowMs) {
    if (g_currentView == AppView::Detail) {
        g_lastOkSignalMs = nowMs;
        if (g_detailExitArmed) {
            Serial.println("OK received after atomizer finish, keep detail alive");
        } else {
            Serial.println("OK received while atomizer is still running");
        }
    }
}

void tftOnBackSignal(uint32_t nowMs) {
    if (g_currentView == AppView::Detail) {
        g_lastGestureHandledMs = nowMs;
        g_lastOkSignalMs = 0;
        g_detailExitArmed = false;
        g_backEvent = false;
        Serial.println("BACK received, returning to current menu");
        setView(AppView::Menu);
    } else if (g_currentView == AppView::Menu) {
        g_lastGestureHandledMs = nowMs;
        g_backEvent = false;
        Serial.println("BACK received in menu, returning to init");
        setView(AppView::Welcome);
    } else {
        Serial.println("BACK ignored in init view");
    }
}

void tftOnAtomizerFinished(uint32_t nowMs) {
    if (g_currentView != AppView::Detail) {
        return;
    }

    g_lastGestureHandledMs = nowMs;
    g_lastOkSignalMs = nowMs;
    g_detailExitArmed = true;
    Serial.println("Atomizer finished, waiting 2s for next OK before returning to menu");
}

void tftToggleScreen(uint32_t nowMs) {
    if (g_currentView == AppView::Welcome) {
        g_currentDishIndex = 0;
        g_lastGestureHandledMs = nowMs;
        Serial.println("UART2 screen toggle: welcome -> menu 1");
        setView(AppView::Menu);
        return;
    }

    if (g_currentView == AppView::Detail) {
        Serial.println("UART2 screen toggle: detail -> menu");
        setView(AppView::Menu);
        return;
    }

    Serial.println("UART2 screen toggle: menu -> welcome");
    setView(AppView::Welcome);
}

void tftResetToWelcome() {
    g_currentDishIndex = 0;
    g_lastGestureHandledMs = 0;
    g_lastOkSignalMs = 0;
    g_detailExitArmed = false;
    g_backEvent = false;
    g_detailEntryEvent = false;
    Serial.println("UART2 reset: back to welcome");
    setView(AppView::Welcome);
}

bool tftIsWelcomeView() {
    return g_currentView == AppView::Welcome;
}

bool tftIsDetailView() {
    return g_currentView == AppView::Detail;
}

uint8_t tftGetExternalMenuSlot() {
    return static_cast<uint8_t>(g_currentDishIndex) + 1;
}

String tftGetExternalScreenCommand() {
    if (g_currentView == AppView::Welcome) {
        return "111";
    }

    uint8_t slot = tftGetExternalMenuSlot();
    if (g_currentView == AppView::Detail) {
        return String(slot) + String(slot);
    }

    return String(slot);
}

bool tftConsumeBackEvent() {
    bool shouldSendBack = g_backEvent;
    g_backEvent = false;
    return shouldSendBack;
}

bool tftConsumeDetailEntryEvent() {
    bool shouldStartAtomizer = g_detailEntryEvent;
    g_detailEntryEvent = false;
    return shouldStartAtomizer;
}
