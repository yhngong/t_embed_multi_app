#include <time.h>
#include <sys/time.h>
#include "config.h"
#include "apa102_led.h"
#include "input.h"
#include "app_launcher.h"
#include "app_flappy.h"
#include "app_shooter.h"
#include "app_snake.h"
#include "app_scope.h"
#include "app_clock.h"
#include "app_system.h"

// Hardware instances
// LGFX_TEmbed incorporates the 35px offset correction and rotation = 3 (wheel on right)
LGFX_TEmbed display;
LGFX_Sprite canvas(&display);
InputManager inputs;
APA102Controller ledBar;

// OS & App instances
AppLauncher launcher;
AppFlappy appFlappy;
AppShooter appShooter;
AppSnake appSnake;
AppScope appScope;
AppClock appClock;
AppSystem appSystem;

int currentApp = APP_LAUNCHER;

// Hardware interrupt handlers for rotary encoder
void IRAM_ATTR onEncoderChange() {
  inputs.tickInterrupt();
}

// Battery percentage calculation from ADC
int getBatteryPercentage() {
  uint32_t raw = analogRead(PIN_BAT_VOLT);
  float vbat = (raw / 4095.0f) * 3.3f * 2.0f;
  return constrain((int)((vbat - 3.3f) / (4.2f - 3.3f) * 100), 0, 100);
}

// Formatted time string helper
String getStatusTime() {
  time_t now = time(nullptr);
  struct tm tInfo;
  localtime_r(&now, &tInfo);
  char buf[16];
  int hr = tInfo.tm_hour % 12;
  if (hr == 0) hr = 12;
  const char* ampm = (tInfo.tm_hour >= 12) ? "PM" : "AM";
  snprintf(buf, sizeof(buf), "%d:%02d %s", hr, tInfo.tm_min, ampm);
  return String(buf);
}

// Initialize system RTC clock from compiler build timestamp
void initSystemTimeFromBuild() {
  struct tm t = {0};
  const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  char s_month[5];
  int day = 1, year = 2026, hour = 0, min = 0, sec = 0;

  sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

  t.tm_year = year - 1900;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_isdst = -1; // Auto DST for Pacific Time
  for (int m = 0; m < 12; m++) {
    if (strncmp(s_month, months[m], 3) == 0) {
      t.tm_mon = m;
      break;
    }
  }

  // Pre-configure Pacific Timezone (San Francisco Bay Area)
  setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
  tzset();

  time_t buildEpoch = mktime(&t);
  struct timeval tv = { .tv_sec = buildEpoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  Serial.printf("System time initialized from build: %s %s -> Epoch: %ld\n", __DATE__, __TIME__, (long)buildEpoch);
}

void setup() {
  // Step 1: Turn on peripheral power rail
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  delay(50);

  // Step 2: Initialize serial logging
  Serial.begin(115200);
  Serial.println("\n=================================");
  Serial.println("  LilyGO T-Embed Multi-App OS");
  Serial.println("  Orientation: Right-Hand Wheel (Rot 3)");
  Serial.println("  ST7789 Offset: 35px corrected");
  Serial.println("=================================");

  // Step 3: Initialize APA102 DotStar LEDs
  ledBar.begin();

  // Step 4: Initialize Rotary Inputs
  inputs.begin();
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODE_A), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODE_B), onEncoderChange, CHANGE);

  // Step 5: Initialize ST7789 Display & Sprite Buffer
  display.begin(3, 230);
  canvas.setColorDepth(16);
  canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  // Step 6: Initialize System Clock (Local San Francisco Bay Area / Pacific Time)
  initSystemTimeFromBuild();

  // Step 7: Launch into Home Launcher
  launcher.begin();
  Serial.println("System Initialized Successfully.");
}

void loop() {
  // Update inputs
  inputs.update();

  if (currentApp == APP_LAUNCHER) {
    int launchTarget = launcher.update();
    if (launchTarget >= 0) {
      currentApp = launchTarget;
      inputs.clearPending();

      switch (currentApp) {
        case APP_FLAPPY:   appFlappy.begin(); break;
        case APP_SHOOTER:  appShooter.begin(); break;
        case APP_SNAKE:    appSnake.begin(); break;
        case APP_SCOPE:    appScope.begin(); break;
        case APP_CLOCK:    appClock.begin(); break;
        case APP_SYSTEM:   appSystem.begin(); break;
      }
    } else {
      launcher.draw(&canvas, getStatusTime().c_str(), getBatteryPercentage());
    }
  } else {
    bool appActive = true;
    switch (currentApp) {
      case APP_FLAPPY:
        appActive = appFlappy.update();
        if (appActive) appFlappy.draw(&canvas);
        break;

      case APP_SHOOTER:
        appActive = appShooter.update();
        if (appActive) appShooter.draw(&canvas);
        break;

      case APP_SNAKE:
        appActive = appSnake.update();
        if (appActive) appSnake.draw(&canvas);
        break;

      case APP_SCOPE:
        appActive = appScope.update();
        if (appActive) appScope.draw(&canvas);
        break;

      case APP_CLOCK:
        appActive = appClock.update();
        if (appActive) appClock.draw(&canvas);
        break;

      case APP_SYSTEM:
        appActive = appSystem.update();
        if (appActive) appSystem.draw(&canvas);
        break;
    }

    // Return to launcher if app exited (e.g. hold knob)
    if (!appActive) {
      currentApp = APP_LAUNCHER;
      setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
      tzset();
      launcher.begin();
      inputs.clearPending();
    }
  }

  // Push frame buffer to display (no margin, fills entire 320x170 screen!)
  canvas.pushSprite(0, 0);

  // Target ~50-60 FPS
  delay(16);
}
