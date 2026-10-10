#pragma once
#include <LilyGo_LovyanGFX.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

class AppScope {
private:
  int channel; // 0: Battery ADC (GPIO 4), 1: ESP32 Die Temp, 2: Synth Audio
  bool isFrozen;
  int timebaseIndex; // 0..5
  static const int BUFFER_SIZE = 240;
  float samples[BUFFER_SIZE];
  int sampleHead;
  uint32_t lastSampleTime;

  float minVal, maxVal, avgVal;

  float readCurrentSignal() {
    if (channel == 0) {
      // Battery Voltage ADC on GPIO 4 (Divider x2, 3.3V reference)
      uint32_t raw = analogRead(PIN_BAT_VOLT);
      float vbat = (raw / 4095.0f) * 3.3f * 2.0f;
      return vbat;
    } else if (channel == 1) {
      // ESP32-S3 Internal Temperature
      return temperatureRead();
    } else {
      // Synthetic signal with harmonics
      float t = millis() / 100.0f;
      return sin(t) * 1.5f + cos(t * 2.3f) * 0.8f + 2.5f;
    }
  }

public:
  AppScope() : channel(0), isFrozen(false), timebaseIndex(2), sampleHead(0), lastSampleTime(0), minVal(0), maxVal(0), avgVal(0) {
    for (int i = 0; i < BUFFER_SIZE; i++) samples[i] = 3.7f;
  }

  void begin() {
    channel = 0;
    isFrozen = false;
    timebaseIndex = 2;
    sampleHead = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) samples[i] = 3.7f;
    ledBar.setBrightness(4);
  }

  bool update() {
    if (inputs.wasLongPressed()) {
      return false; // Exit to launcher
    }

    // Rotary Wheel Adjusts Timebase (or Channel on quick click)
    int delta = inputs.getDelta();
    if (delta != 0) {
      timebaseIndex += delta;
      if (timebaseIndex < 0) timebaseIndex = 0;
      if (timebaseIndex > 5) timebaseIndex = 5;
    }

    // Click Knob: Toggles Channel or Freeze
    if (inputs.wasClicked()) {
      channel = (channel + 1) % 3;
      ledBar.flash(0x00FF88, 8);
    }

    // Sampling loop
    const uint32_t INTERVALS[6] = { 2, 5, 10, 25, 50, 100 };
    uint32_t interval = INTERVALS[timebaseIndex];

    if (!isFrozen && (millis() - lastSampleTime >= interval)) {
      lastSampleTime = millis();
      float val = readCurrentSignal();
      samples[sampleHead] = val;
      sampleHead = (sampleHead + 1) % BUFFER_SIZE;

      // Calculate statistics
      minVal = 9999.0f;
      maxVal = -9999.0f;
      float sum = 0.0f;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        if (samples[i] < minVal) minVal = samples[i];
        if (samples[i] > maxVal) maxVal = samples[i];
        sum += samples[i];
      }
      avgVal = sum / BUFFER_SIZE;

      // Update LED VU Meter
      float normalized = (maxVal > minVal) ? (val - minVal) / (maxVal - minVal) : 0.5f;
      ledBar.setVUMeter(normalized);
    }

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    canvas->fillScreen(0x0020); // Deep CRT phosphor dark

    // Scope Graticule Area (240 x 120, centered at x=10, y=24)
    const int GRID_X = 10;
    const int GRID_Y = 24;
    const int GRID_W = 220;
    const int GRID_H = 114;

    canvas->drawRoundRect(GRID_X - 1, GRID_Y - 1, GRID_W + 2, GRID_H + 2, 4, 0x03E0);
    canvas->fillRect(GRID_X, GRID_Y, GRID_W, GRID_H, 0x0000);

    // Graticule Dotted Grid
    for (int x = GRID_X + 22; x < GRID_X + GRID_W; x += 22) {
      for (int y = GRID_Y + 2; y < GRID_Y + GRID_H; y += 4) {
        canvas->drawPixel(x, y, 0x0200);
      }
    }
    for (int y = GRID_Y + 19; y < GRID_Y + GRID_H; y += 19) {
      for (int x = GRID_X + 2; x < GRID_X + GRID_W; x += 4) {
        canvas->drawPixel(x, y, 0x0200);
      }
    }
    // Center reticle axes
    canvas->drawFastHLine(GRID_X, GRID_Y + (GRID_H / 2), GRID_W, 0x0360);
    canvas->drawFastVLine(GRID_X + (GRID_W / 2), GRID_Y, GRID_H, 0x0360);

    // Draw Signal Waveform Trace
    float range = (maxVal - minVal);
    if (range < 0.1f) range = 0.5f;

    int prevY = -1;
    for (int i = 0; i < GRID_W; i++) {
      int idx = (sampleHead - GRID_W + i + BUFFER_SIZE) % BUFFER_SIZE;
      float sample = samples[idx];
      int plotY = GRID_Y + GRID_H - 10 - (int)(((sample - minVal) / range) * (GRID_H - 20));
      plotY = constrain(plotY, GRID_Y + 2, GRID_Y + GRID_H - 2);

      int plotX = GRID_X + i;
      if (prevY != -1) {
        canvas->drawLine(plotX - 1, prevY, plotX, plotY, 0x07E0);
        canvas->drawPixel(plotX, plotY, 0x9FE0); // Bright trace core
      }
      prevY = plotY;
    }

    // Header Status Bar
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0x07E0, 0x0020);
    canvas->drawString("CRT OSCILLOSCOPE", 10, 6, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::top_center);
    const char* chanNames[3] = { "CH1: VBAT (GPIO 4)", "CH2: DIE TEMP", "CH3: SYNTH WAVE" };
    canvas->setTextColor(0x07FF, 0x0020);
    canvas->drawString(chanNames[channel], 150, 6, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::top_right);
    canvas->setTextColor(isFrozen ? 0xF800 : 0x07E0, 0x0020);
    canvas->drawString(isFrozen ? "[HOLD]" : "[RUN]", 310, 6, &fonts::Font2);

    // Right Telemetry Panel (x=236..315)
    int px = 240;
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0x7BEF, 0x0020);
    canvas->drawString("METRICS", px, 26, &fonts::Font0);

    canvas->setTextColor(0xFFFF, 0x0020);
    if (channel == 0) {
      canvas->drawString("Vavg: " + String(avgVal, 2) + "V", px, 40, &fonts::Font0);
      canvas->drawString("Vmax: " + String(maxVal, 2) + "V", px, 54, &fonts::Font0);
      canvas->drawString("Vmin: " + String(minVal, 2) + "V", px, 68, &fonts::Font0);
      canvas->drawString("Vpp:  " + String((maxVal - minVal) * 1000, 0) + "mV", px, 82, &fonts::Font0);
    } else if (channel == 1) {
      canvas->drawString("Temp: " + String(avgVal, 1) + "C", px, 40, &fonts::Font0);
      canvas->drawString("Farh: " + String(avgVal * 1.8f + 32.0f, 1) + "F", px, 54, &fonts::Font0);
      canvas->drawString("Max:  " + String(maxVal, 1) + "C", px, 68, &fonts::Font0);
      canvas->drawString("Min:  " + String(minVal, 1) + "C", px, 82, &fonts::Font0);
    } else {
      canvas->drawString("Sig:  Synth", px, 40, &fonts::Font0);
      canvas->drawString("Pk:   " + String(maxVal, 2), px, 54, &fonts::Font0);
      canvas->drawString("Avg:  " + String(avgVal, 2), px, 68, &fonts::Font0);
      canvas->drawString("RMS:  " + String(range * 0.707f, 2), px, 82, &fonts::Font0);
    }

    const char* tbNames[6] = { "2ms/div", "5ms/div", "10ms/div", "25ms/div", "50ms/div", "100ms/div" };
    canvas->setTextColor(0xFD00, 0x0020);
    canvas->drawString("TB: " + String(tbNames[timebaseIndex]), px, 102, &fonts::Font0);

    canvas->setTextColor(0x07FF, 0x0020);
    canvas->drawString("VU Meter ON", px, 118, &fonts::Font0);

    // Bottom Navigation Bar
    canvas->drawFastHLine(10, 144, 300, 0x0360);
    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0x7BEF, 0x0020);
    canvas->drawString("Dial: Sweep Time   *   Click: Switch CH   *   Hold: Menu", 160, 156, &fonts::Font0);
  }
};
