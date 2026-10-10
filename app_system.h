#pragma once
#include <LilyGo_LovyanGFX.h>
#include <WiFi.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct WifiApEntry {
  String ssid;
  int32_t rssi;
  uint8_t channel;
  wifi_auth_mode_t encryption;
};

class AppSystem {
private:
  int scrollOffset;
  int scanCount;
  static const int MAX_APS = 16;
  WifiApEntry apList[MAX_APS];
  bool isScanning;
  String scanMessage;
  uint32_t lastScanTime;

  void runWifiScan() {
    isScanning = true;
    scanMessage = "Scanning 2.4GHz bands...";
    ledBar.setAll(0x002255);
    ledBar.show();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks(false, false, false, 250);
    scanCount = min(n, MAX_APS);
    for (int i = 0; i < scanCount; i++) {
      apList[i].ssid = WiFi.SSID(i);
      apList[i].rssi = WiFi.RSSI(i);
      apList[i].channel = WiFi.channel(i);
      apList[i].encryption = WiFi.encryptionType(i);
    }
    WiFi.scanDelete();

    scanMessage = "Found " + String(n) + " networks";
    isScanning = false;
    lastScanTime = millis();
    ledBar.flash(0x3399FF, 10);
  }

public:
  AppSystem() : scrollOffset(0), scanCount(0), isScanning(false), scanMessage("Click to Scan Wi-Fi"), lastScanTime(0) {}

  void begin() {
    scrollOffset = 0;
    scanCount = 0;
    isScanning = false;
    scanMessage = "Click knob to scan Wi-Fi";
    ledBar.setAll(0x001144);
    ledBar.show();
  }

  bool update() {
    if (inputs.wasLongPressed()) {
      return false; // Exit to launcher
    }

    // Rotary Wheel Scrolls Content
    int delta = inputs.getDelta();
    if (delta != 0) {
      scrollOffset += delta * 12;
      int maxScroll = max(0, 140 + (scanCount * 18) - 130);
      scrollOffset = constrain(scrollOffset, 0, maxScroll);
      ledBar.setDialChase(scrollOffset / 12, 0x3399FF);
    }

    // Click Knob: Trigger Wi-Fi Scan
    if (inputs.wasClicked() && !isScanning) {
      runWifiScan();
    }

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    canvas->fillScreen(0x0821);

    // Fixed Top Header (320 x 24)
    canvas->fillRoundRect(6, 4, 308, 22, 4, 0x10A4);
    canvas->setTextDatum(textdatum_t::middle_left);
    canvas->setTextColor(0x3CDA, 0x10A4);
    canvas->drawString("HARDWARE TELEMETRY & WI-FI", 14, 15, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::middle_right);
    canvas->setTextColor(0xFFFF, 0x10A4);
    canvas->drawString(scanMessage, 304, 15, &fonts::Font0);

    // Scrollable Viewport (Y: 30 .. 142)
    int yBase = 32 - scrollOffset;

    // --- Section 1: SoC & Memory ---
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0x07FF, 0x0821);
    canvas->drawString("=== ESP32-S3 HARDWARE SPECS ===", 10, yBase, &fonts::Font2);

    canvas->setTextColor(0xFFFF, 0x0821);
    canvas->drawString("CPU Freq: 240 MHz (Dual Core Xtensa)", 10, yBase + 18, &fonts::Font0);
    canvas->drawString("Flash Size: 16 MB Quad SPI", 10, yBase + 30, &fonts::Font0);
    canvas->drawString("PSRAM: 8 MB Octal (Free: " + String(ESP.getFreePsram() / 1024) + " KB)", 10, yBase + 42, &fonts::Font0);
    canvas->drawString("Heap SRAM: Free " + String(ESP.getFreeHeap() / 1024) + " KB / Total 320 KB", 10, yBase + 54, &fonts::Font0);

    // Battery ADC
    uint32_t raw = analogRead(PIN_BAT_VOLT);
    float vbat = (raw / 4095.0f) * 3.3f * 2.0f;
    int batPct = constrain((int)((vbat - 3.3f) / (4.2f - 3.3f) * 100), 0, 100);
    canvas->drawString("Battery (GPIO 4): " + String(vbat, 2) + "V  (" + String(batPct) + "%)", 10, yBase + 66, &fonts::Font0);

    // Wheel Diagnostics
    canvas->setTextColor(0x07E0, 0x0821);
    canvas->drawString("Wheel Pos: " + String(inputs.getAbsolutePosition()) + " ticks  |  Knob: " + (inputs.isKeyDown() ? "DOWN" : "UP"), 10, yBase + 78, &fonts::Font0);

    // --- Section 2: Wi-Fi Networks ---
    canvas->setTextColor(0xFD00, 0x0821);
    canvas->drawString("=== SCANNED 2.4GHz NETWORKS (" + String(scanCount) + ") ===", 10, yBase + 96, &fonts::Font2);

    if (scanCount == 0) {
      canvas->setTextColor(0x7BEF, 0x0821);
      canvas->drawString("No networks scanned yet. Click knob to run scan.", 10, yBase + 116, &fonts::Font0);
    } else {
      for (int i = 0; i < scanCount; i++) {
        int rowY = yBase + 116 + (i * 18);
        if (rowY > 16 && rowY < 150) {
          // SSID
          canvas->setTextColor(0xFFFF, 0x0821);
          String shortSsid = apList[i].ssid;
          if (shortSsid.length() > 18) shortSsid = shortSsid.substring(0, 17) + "..";
          if (shortSsid.length() == 0) shortSsid = "[Hidden SSID]";
          canvas->drawString(String(i + 1) + ". " + shortSsid, 10, rowY, &fonts::Font0);

          // RSSI bar & text
          int r = apList[i].rssi;
          uint16_t sigColor = (r > -60) ? 0x07E0 : ((r > -75) ? 0xFD00 : 0xF800);
          canvas->setTextColor(sigColor, 0x0821);
          canvas->drawString(String(r) + " dBm", 210, rowY, &fonts::Font0);

          // Channel
          canvas->setTextColor(0x7BEF, 0x0821);
          canvas->drawString("CH " + String(apList[i].channel), 275, rowY, &fonts::Font0);
        }
      }
    }

    // Fixed Bottom Navigation Bar (320 x 24)
    canvas->fillRoundRect(6, 144, 308, 22, 4, 0x10A4);
    canvas->drawFastHLine(10, 144, 300, 0x2145);
    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0x7BEF, 0x10A4);
    canvas->drawString("Dial: Scroll View   *   Click: Scan Wi-Fi   *   Hold: Menu", 160, 155, &fonts::Font0);
  }
};
