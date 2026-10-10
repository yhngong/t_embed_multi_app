#pragma once
#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

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

// ST7789 Display SPI Pins
#define PIN_LCD_SCLK    12
#define PIN_LCD_MOSI    11
#define PIN_LCD_MISO    -1
#define PIN_LCD_DC      13
#define PIN_LCD_CS      10
#define PIN_LCD_RST      9
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

// Dedicated LovyanGFX device definition for T-Embed:
// Corrects the 35px ST7789 hardware offset (240 - 170 = 70 / 2 = 35)
// and sets rotation = 3 so the screen is right-side-up with the wheel on the RIGHT for right-handed use!
class LGFX_TEmbed : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;
  lgfx::Light_PWM    _light_instance;

public:
  LGFX_TEmbed() {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = SPI_MODE0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 20000000;
      cfg.pin_sclk   = PIN_LCD_SCLK;
      cfg.pin_mosi   = PIN_LCD_MOSI;
      cfg.pin_miso   = PIN_LCD_MISO;
      cfg.pin_dc     = PIN_LCD_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs        = PIN_LCD_CS;
      cfg.pin_rst       = PIN_LCD_RST;
      cfg.pin_busy      = -1;
      // ST7789 driver memory width is 240, physical glass is 170.
      // Column offset is 35!
      cfg.memory_width  = 240;
      cfg.memory_height = 320;
      cfg.panel_width   = 170;
      cfg.panel_height  = 320;
      cfg.offset_x      = 35;
      cfg.offset_y      = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.readable      = false;
      cfg.invert        = true;
      cfg.rgb_order     = false;
      cfg.bus_shared    = true;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = PIN_LCD_BL;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
  }

  bool begin(uint8_t rotation = 3, uint8_t brightness = 230) {
    bool ok = init();
    setRotation(rotation); // Rotation 3: Wheel on right, right-side-up, no margin!
    setBrightness(brightness);
    fillScreen(TFT_BLACK);
    return ok;
  }
};
