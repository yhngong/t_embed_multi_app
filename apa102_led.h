#pragma once
#include <Arduino.h>
#include "config.h"

// Physical clockwise mapping of APA102 LEDs around T-Embed rotary knob
const uint8_t LED_MAP[NUM_LEDS] = { 2, 1, 0, 6, 5, 4, 3 };

class APA102Controller {
private:
  uint32_t currentColors[NUM_LEDS];
  uint8_t globalBrightness;

  void sendByte(uint8_t b) {
    for (int i = 7; i >= 0; i--) {
      digitalWrite(PIN_APA102_DI, (b & (1 << i)) ? HIGH : LOW);
      digitalWrite(PIN_APA102_CLK, HIGH);
      delayMicroseconds(1);
      digitalWrite(PIN_APA102_CLK, LOW);
    }
  }

public:
  APA102Controller() : globalBrightness(4) {
    for (int i = 0; i < NUM_LEDS; i++) currentColors[i] = 0;
  }

  void begin() {
    pinMode(PIN_APA102_DI, OUTPUT);
    pinMode(PIN_APA102_CLK, OUTPUT);
    digitalWrite(PIN_APA102_DI, LOW);
    digitalWrite(PIN_APA102_CLK, LOW);
    clear();
  }

  void setBrightness(uint8_t brightness) {
    if (brightness > 31) brightness = 31;
    globalBrightness = brightness;
  }

  void setLED(int index, uint32_t color) {
    if (index >= 0 && index < NUM_LEDS) {
      currentColors[index] = color;
    }
  }

  void setAll(uint32_t color) {
    for (int i = 0; i < NUM_LEDS; i++) {
      currentColors[i] = color;
    }
  }

  void clear() {
    setAll(0);
    show();
  }

  void show() {
    // 32 zero bits start frame
    for (int i = 0; i < 4; i++) sendByte(0x00);

    // 7 LED frames: 0xE0 | brightness, B, G, R
    uint8_t header = 0xE0 | (globalBrightness & 0x1F);
    for (int i = 0; i < NUM_LEDS; i++) {
      // Map logical LED position to physical LED order around the knob
      uint8_t physicalIdx = LED_MAP[i];
      uint8_t r = (currentColors[physicalIdx] >> 16) & 0xFF;
      uint8_t g = (currentColors[physicalIdx] >> 8) & 0xFF;
      uint8_t b = currentColors[physicalIdx] & 0xFF;
      sendByte(header);
      sendByte(b);
      sendByte(g);
      sendByte(r);
    }

    // 32 one bits end frame
    for (int i = 0; i < 4; i++) sendByte(0xFF);
  }

  // Effect: Circular rotary chase
  void setDialChase(int position, uint32_t primaryColor, uint32_t trailColor = 0x001122) {
    int active = ((position % NUM_LEDS) + NUM_LEDS) % NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i == active) {
        currentColors[i] = primaryColor;
      } else if ((i + 1) % NUM_LEDS == active || (i + NUM_LEDS - 1) % NUM_LEDS == active) {
        currentColors[i] = trailColor;
      } else {
        currentColors[i] = 0x000000;
      }
    }
    show();
  }

  // Effect: VU Meter bar
  void setVUMeter(float level, uint32_t lowColor = 0x00FF00, uint32_t highColor = 0xFF0000) {
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    int litCount = (int)(level * NUM_LEDS + 0.5f);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < litCount) {
        if (i < 4) currentColors[i] = 0x00FF00;      // Green
        else if (i < 6) currentColors[i] = 0xFFAA00; // Yellow/Amber
        else currentColors[i] = 0xFF0000;            // Red
      } else {
        currentColors[i] = 0x000000;
      }
    }
    show();
  }

  // Effect: Flash burst (e.g. game hit or point scored)
  void flash(uint32_t color, uint8_t brightness = 15) {
    uint8_t prevBright = globalBrightness;
    globalBrightness = brightness;
    setAll(color);
    show();
    globalBrightness = prevBright;
  }
};

extern APA102Controller ledBar;
