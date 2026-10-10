#pragma once
#include <LilyGo_LovyanGFX.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct AppMetadata {
  const char* title;
  const char* subtitle;
  uint16_t color;
  uint32_t rgbColor;
};

const AppMetadata APP_INFO[APP_COUNT] = {
  { "Flappy Bird",    "Wheel / Tap Flap Game",     0xFD00, 0xFFAA00 }, // Amber
  { "Space Shooter",  "Analog Spinner Arcade",     0x07FF, 0x00D9FF }, // Cyan
  { "Retro Snake",    "Rotary Turning Nibbler",    0x07E0, 0x00FF66 }, // Green
  { "Sensor Scope",   "CRT Waveform & Battery",    0x067A, 0x00E5AA }, // Emerald
  { "World Clock",    "NTP & Timezone Dial",       0xB9BF, 0xAA66FF }, // Purple
  { "Sys Telemetry",  "ESP32-S3 & Wi-Fi Scanner",  0x3CDA, 0x3399FF }  // Electric Blue
};

class AppLauncher {
private:
  int selectedIndex;
  float currentScroll;
  uint32_t lastAnimTime;

  void drawMiniIcon(LGFX_Sprite* canvas, int x, int y, int appId) {
    switch (appId) {
      case 0: // Flappy Bird
        canvas->fillCircle(x, y, 16, 0xFD00);
        canvas->fillCircle(x + 5, y - 4, 4, 0xFFFF);
        canvas->fillCircle(x + 6, y - 4, 2, 0x0000);
        canvas->fillTriangle(x + 10, y, x + 19, y + 2, x + 10, y + 5, 0xF980);
        canvas->fillRoundRect(x - 12, y + 1, 11, 7, 3, 0xFFFF);
        break;

      case 1: // Space Shooter
        canvas->fillTriangle(x + 14, y, x - 12, y - 11, x - 12, y + 11, 0x07FF);
        canvas->fillTriangle(x - 2, y, x - 15, y - 5, x - 15, y + 5, 0xF800);
        canvas->fillCircle(x + 2, y, 3, 0xFFFF);
        canvas->drawLine(x + 16, y - 4, x + 24, y - 4, 0xFFE0);
        canvas->drawLine(x + 16, y + 4, x + 24, y + 4, 0xFFE0);
        break;

      case 2: // Retro Snake
        for (int i = 0; i < 4; i++) {
          canvas->fillRoundRect(x - 14 + i * 7, y - 3 + ((i % 2) ? -3 : 3), 6, 6, 2, 0x07E0);
        }
        canvas->fillCircle(x + 12, y - 4, 5, 0xF800); // Apple
        canvas->drawLine(x + 12, y - 9, x + 14, y - 12, 0x07E0); // Stem
        break;

      case 3: // Sensor Scope
        canvas->drawRoundRect(x - 16, y - 12, 32, 24, 4, 0x07E0);
        for (int px = -12; px <= 12; px += 2) {
          int py = (int)(sin((px + millis() / 150.0f) * 0.4f) * 7.0f);
          canvas->drawPixel(x + px, y + py, 0x07FF);
        }
        break;

      case 4: // World Clock
        canvas->drawCircle(x, y, 15, 0xFFFF);
        canvas->drawCircle(x, y, 16, 0xB9BF);
        canvas->fillCircle(x, y, 2, 0xFFFF);
        canvas->drawLine(x, y, x + 8, y, 0xFFFF);
        canvas->drawLine(x, y, x, y - 10, 0xF800);
        break;

      case 5: // Sys Telemetry
        canvas->drawRoundRect(x - 14, y - 12, 28, 24, 3, 0x3CDA);
        canvas->drawFastHLine(x - 10, y - 5, 20, 0x07E0);
        canvas->drawFastHLine(x - 10, y, 14, 0xFD00);
        canvas->drawFastHLine(x - 10, y + 5, 18, 0x07FF);
        break;
    }
  }

public:
  AppLauncher() : selectedIndex(0), currentScroll(0.0f), lastAnimTime(0) {}

  void begin() {
    selectedIndex = 0;
    currentScroll = 0.0f;
    lastAnimTime = millis();
    ledBar.setBrightness(4);
    ledBar.setDialChase(selectedIndex, APP_INFO[selectedIndex].rgbColor);
  }

  int update() {
    // Read rotary encoder input
    int delta = inputs.getDelta();
    if (delta != 0) {
      selectedIndex = (selectedIndex + delta) % APP_COUNT;
      if (selectedIndex < 0) selectedIndex += APP_COUNT;
      ledBar.setDialChase(selectedIndex, APP_INFO[selectedIndex].rgbColor);
    }

    // Smooth lerp scrolling animation
    float targetScroll = selectedIndex;
    currentScroll += (targetScroll - currentScroll) * 0.28f;

    // Check for app launch (button clicked)
    if (inputs.wasClicked()) {
      ledBar.flash(APP_INFO[selectedIndex].rgbColor, 12);
      return selectedIndex;
    }

    return -1; // Remain in launcher
  }

  void draw(LGFX_Sprite* canvas, const char* timeStr, int batteryPercent) {
    // Background gradient
    canvas->fillScreen(0x0842); // Deep Space Navy
    canvas->fillRoundRect(4, 2, 312, 166, 8, 0x0821);

    // ==========================================
    // Top Status Bar (320 x 22)
    // ==========================================
    canvas->setTextDatum(textdatum_t::middle_left);
    canvas->setTextColor(0xFFFF, 0x0821);
    canvas->drawString("T-Embed OS", 14, 13, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0x07FF, 0x0821);
    canvas->drawString(timeStr, 160, 13, &fonts::Font2);

    // Battery pill icon on right
    int bx = 268, by = 7;
    canvas->drawRoundRect(bx, by, 28, 12, 3, 0x7BEF);
    canvas->fillRoundRect(bx + 28, by + 3, 2, 6, 1, 0x7BEF);
    int fillW = map(constrain(batteryPercent, 0, 100), 0, 100, 0, 24);
    uint16_t bColor = (batteryPercent > 20) ? 0x07E0 : 0xF800;
    if (fillW > 0) canvas->fillRoundRect(bx + 2, by + 2, fillW, 8, 2, bColor);
    canvas->setTextDatum(textdatum_t::middle_right);
    canvas->setTextColor(0x7BEF, 0x0821);
    canvas->drawString(String(batteryPercent) + "%", bx - 4, 13, &fonts::Font0);

    // Divider line below header
    canvas->drawFastHLine(10, 24, 300, 0x18E5);

    // ==========================================
    // Dynamic Carousel of App Cards
    // ==========================================
    const int CARD_WIDTH = 196;
    const int CARD_HEIGHT = 100;
    const int CARD_Y = 32;
    const int CARD_SPACING = 216;

    for (int i = -1; i <= 1; i++) {
      int cardIndex = (selectedIndex + i + APP_COUNT) % APP_COUNT;
      float cardOffset = (selectedIndex + i) - currentScroll;
      int cardX = (SCREEN_WIDTH / 2) + (int)(cardOffset * CARD_SPACING) - (CARD_WIDTH / 2);

      if (cardX > -CARD_WIDTH && cardX < SCREEN_WIDTH) {
        bool isCenter = (i == 0);
        uint16_t cardBg = isCenter ? 0x10A4 : 0x0862;
        uint16_t borderColor = isCenter ? APP_INFO[cardIndex].color : 0x2967;

        // Card frame
        canvas->fillRoundRect(cardX, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 8, cardBg);
        canvas->drawRoundRect(cardX, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 8, borderColor);
        if (isCenter) {
          // Double glowing highlight ring for active card
          canvas->drawRoundRect(cardX - 1, CARD_Y - 1, CARD_WIDTH + 2, CARD_HEIGHT + 2, 9, borderColor);
        }

        // App Icon
        drawMiniIcon(canvas, cardX + 34, CARD_Y + 36, cardIndex);

        // App Title
        canvas->setTextDatum(textdatum_t::top_left);
        canvas->setTextColor(isCenter ? 0xFFFF : 0x9CD3, cardBg);
        canvas->drawString(APP_INFO[cardIndex].title, cardX + 66, CARD_Y + 18, &fonts::Font2);

        // App Subtitle / Mode
        canvas->setTextColor(isCenter ? APP_INFO[cardIndex].color : 0x632C, cardBg);
        canvas->drawString(APP_INFO[cardIndex].subtitle, cardX + 66, CARD_Y + 38, &fonts::Font0);

        // Card Footer / Action pill
        if (isCenter) {
          canvas->fillRoundRect(cardX + 20, CARD_Y + 70, CARD_WIDTH - 40, 20, 5, 0x0000);
          canvas->drawRoundRect(cardX + 20, CARD_Y + 70, CARD_WIDTH - 40, 20, 5, borderColor);
          canvas->setTextDatum(textdatum_t::middle_center);
          canvas->setTextColor(0xFFFF, 0x0000);
          canvas->drawString("PRESS KNOB TO LAUNCH", cardX + (CARD_WIDTH / 2), CARD_Y + 80, &fonts::Font0);
        }
      }
    }

    // ==========================================
    // Bottom Navigation Hint Footer
    // ==========================================
    canvas->drawFastHLine(10, 142, 300, 0x18E5);
    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0x7BEF, 0x0821);
    canvas->drawString("Rotary Dial: Browse   |   Click: Select   |   Hold: Home", 160, 154, &fonts::Font0);
  }
};
