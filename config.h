#pragma once
#include <Arduino.h>

// ==========================================
// LilyGO T-Embed Hardware Pin Definitions
// ==========================================

// Peripheral Power Enable (Must be HIGH to power LCD, LEDs, SD, Audio)
#define PIN_POWER_ON    46

// Rotary Encoder & Button
#define PIN_ENCODE_A     2
#define PIN_ENCODE_B     1
#define PIN_ENCODE_BTN   0

// APA102 DotStar RGB LEDs (7 LEDs along front dial)
#define PIN_APA102_DI   42
#define PIN_APA102_CLK  45
#define NUM_LEDS         7

// ST7789 Display SPI Pins (handled by LilyGo_T_Embed LovyanGFX class)
// SCLK: 12, MOSI: 11, MISO: -1, DC: 13, CS: 10, RST: 9, BL: 15
#define PIN_LCD_BL      15

// Battery ADC
#define PIN_BAT_VOLT     4

// I2S Audio Amp (MAX98357A)
#define PIN_I2S_BCLK     7
#define PIN_I2S_LRCK     5
#define PIN_I2S_DOUT     6

// Screen Dimensions (Landscape 320 x 170)
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  170

// App Identifiers
enum AppId {
  APP_LAUNCHER = -1,
  APP_FLAPPY = 0,
  APP_SHOOTER = 1,
  APP_SNAKE = 2,
  APP_SCOPE = 3,
  APP_CLOCK = 4,
  APP_SYSTEM = 5,
  APP_COUNT = 6
};

// 16-bit 565 Color Palette
#define COLOR_BG_DARK       0x0842
#define COLOR_CARD_BG       0x18E5
#define COLOR_ACCENT_CYAN   0x07FF
#define COLOR_ACCENT_AMBER  0xFD00
#define COLOR_ACCENT_GREEN  0x07E0
#define COLOR_ACCENT_RED    0xF800
#define COLOR_ACCENT_BLUE   0x3A9F
#define COLOR_ACCENT_PURPLE 0xB19F
#define COLOR_TEXT_MUTED    0x8410
#define COLOR_TEXT_WHITE    0xFFFF
