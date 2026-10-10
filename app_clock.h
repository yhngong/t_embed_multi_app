#pragma once
#include <LilyGo_LovyanGFX.h>
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct TimeZoneInfo {
  const char* city;
  const char* code;
  const char* tzConfig;
  int gmtOffsetHours;
};

const TimeZoneInfo WORLD_ZONES[7] = {
  { "San Francisco", "PT",  "PST8PDT,M3.2.0,M11.1.0", -7 },
  { "New York",      "ET",  "EST5EDT,M3.2.0,M11.1.0", -4 },
  { "London",        "GMT", "GMT0BST,M3.5.0/1,M10.5.0", 1 },
  { "Paris",         "CET", "CET-1CEST,M3.5.0,M10.5.0/3", 2 },
  { "Tokyo",         "JST", "JST-9",                   9 },
  { "Sydney",        "AEST","AEST-10AEDT,M10.1.0,M4.1.0/3", 11 },
  { "Honolulu",      "HST", "HST10",                  -10 }
};

class AppClock {
private:
  int zoneIndex;
  bool isSyncing;
  String syncStatus;
  uint32_t lastTickTime;

public:
  AppClock() : zoneIndex(0), isSyncing(false), syncStatus("RTC Active"), lastTickTime(0) {}

  void begin() {
    zoneIndex = 0;
    isSyncing = false;
    syncStatus = "RTC (SF Bay Area)";
    setenv("TZ", WORLD_ZONES[zoneIndex].tzConfig, 1);
    tzset();
    ledBar.setBrightness(3);
    ledBar.setAll(0x110033);
    ledBar.show();
  }

  void startNtpSync() {
    isSyncing = true;
    syncStatus = "Scanning Wi-Fi...";

    Preferences netPrefs;
    netPrefs.begin("wifi_cfg", true);
    String ssid = netPrefs.getString("ssid", "");
    String pass = netPrefs.getString("pass", "");
    netPrefs.end();

    WiFi.mode(WIFI_STA);
    if (ssid.length() > 0) {
      WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
      WiFi.begin();
    }

    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 3500) {
      delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
      syncStatus = "Syncing NTP atomic...";
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      setenv("TZ", WORLD_ZONES[zoneIndex].tzConfig, 1);
      tzset();
      syncStatus = "NTP Synced OK!";
      ledBar.flash(0x00FF88, 10);
    } else {
      syncStatus = "RTC Active (No Wi-Fi)";
      ledBar.flash(0xFF8800, 8);
    }
    isSyncing = false;
  }

  bool update() {
    if (inputs.wasLongPressed()) {
      return false; // Exit to launcher
    }

    // Rotary Wheel Dials through Timezones!
    int delta = inputs.getDelta();
    if (delta != 0) {
      zoneIndex = (zoneIndex + delta) % 7;
      if (zoneIndex < 0) zoneIndex += 7;
      setenv("TZ", WORLD_ZONES[zoneIndex].tzConfig, 1);
      tzset();
      ledBar.setDialChase(zoneIndex, 0xAA66FF);
    }

    // Click Knob: Sync NTP
    if (inputs.wasClicked() && !isSyncing) {
      startNtpSync();
    }

    // Breathing LED glow
    uint32_t t = millis();
    int bright = (int)(sin(t / 800.0f) * 3.0f + 4.0f);
    ledBar.setBrightness(bright);

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    canvas->fillScreen(0x0822); // Deep midnight slate

    time_t now = time(nullptr);
    struct tm tInfo;
    localtime_r(&now, &tInfo);

    // ==========================================
    // Left: Analog Clock Face (Radius 48, Center: 68, 85)
    // ==========================================
    const int CX = 68;
    const int CY = 85;
    const int CR = 48;

    canvas->fillCircle(CX, CY, CR, 0x10A4);
    canvas->drawCircle(CX, CY, CR, 0x39E7);
    canvas->drawCircle(CX, CY, CR - 1, 0x738E);

    // 12 Hour tick marks
    for (int h = 0; h < 12; h++) {
      float ang = h * (360.0f / 12.0f) * 0.0174533f;
      int r1 = (h % 3 == 0) ? CR - 8 : CR - 4;
      int x1 = CX + (int)(sin(ang) * r1);
      int y1 = CY - (int)(cos(ang) * r1);
      int x2 = CX + (int)(sin(ang) * CR);
      int y2 = CY - (int)(cos(ang) * CR);
      canvas->drawLine(x1, y1, x2, y2, (h % 3 == 0) ? 0xFFFF : 0x7BEF);
    }

    // Hour Hand
    float hAngle = (tInfo.tm_hour % 12 + tInfo.tm_min / 60.0f) * 30.0f * 0.0174533f;
    int hx = CX + (int)(sin(hAngle) * 26);
    int hy = CY - (int)(cos(hAngle) * 26);
    canvas->drawLine(CX, CY, hx, hy, 0xFFFF);
    canvas->drawLine(CX + 1, CY, hx + 1, hy, 0xFFFF);

    // Minute Hand
    float mAngle = (tInfo.tm_min + tInfo.tm_sec / 60.0f) * 6.0f * 0.0174533f;
    int mx = CX + (int)(sin(mAngle) * 36);
    int my = CY - (int)(cos(mAngle) * 36);
    canvas->drawLine(CX, CY, mx, my, 0x07FF);
    canvas->drawLine(CX + 1, CY, mx + 1, my, 0x07FF);

    // Second Hand
    float sAngle = tInfo.tm_sec * 6.0f * 0.0174533f;
    int sx = CX + (int)(sin(sAngle) * 40);
    int sy = CY - (int)(cos(sAngle) * 40);
    canvas->drawLine(CX, CY, sx, sy, 0xF800);
    canvas->fillCircle(CX, CY, 3, 0xF800);

    // ==========================================
    // Right: Digital Clock & World Timezone Card
    // ==========================================
    int rx = 136;

    // Timezone badge
    canvas->fillRoundRect(rx, 14, 172, 22, 4, 0x18C5);
    canvas->drawRoundRect(rx, 14, 172, 22, 4, 0xB9BF);
    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0xFFFF, 0x18C5);
    canvas->drawString(String(WORLD_ZONES[zoneIndex].city) + " (" + String(WORLD_ZONES[zoneIndex].code) + ")", rx + 86, 25, &fonts::Font2);

    // Large Digital Time
    char timeBuf[16];
    int hr = tInfo.tm_hour;
    const char* ampm = (hr >= 12) ? "PM" : "AM";
    int displayHr = hr % 12;
    if (displayHr == 0) displayHr = 12;
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", displayHr, tInfo.tm_min, tInfo.tm_sec);

    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0x07FF, 0x0822);
    canvas->drawString(timeBuf, rx + 4, 42, &fonts::Font4);

    canvas->setTextColor(0xFD00, 0x0822);
    canvas->drawString(ampm, rx + 144, 44, &fonts::Font2);

    // Date String (Properly positioned at y=76 with ample space)
    char dateBuf[32];
    const char* days[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    const char* months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    snprintf(dateBuf, sizeof(dateBuf), "%s, %s %d, %d", 
             days[tInfo.tm_wday], months[tInfo.tm_mon], tInfo.tm_mday, tInfo.tm_year + 1900);

    canvas->setTextColor(0xFFFF, 0x0822);
    canvas->drawString(dateBuf, rx + 4, 76, &fonts::Font2);

    // Status / Timezone info
    canvas->setTextColor(0x7BEF, 0x0822);
    canvas->drawString("Status: " + syncStatus, rx + 4, 102, &fonts::Font0);

    // Bottom Navigation Bar
    canvas->drawFastHLine(10, 144, 300, 0x18C5);
    canvas->setTextDatum(textdatum_t::middle_center);
    canvas->setTextColor(0x7BEF, 0x0822);
    canvas->drawString("Dial: Switch City   *   Click: NTP Sync   *   Hold: Menu", 160, 156, &fonts::Font0);
  }
};
