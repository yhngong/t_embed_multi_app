# LilyGO T-Embed Multi-App OS 🎮

A modular, multi-app firmware for the **LilyGO T-Embed** (ESP32-S3) microcontroller featuring a 3D rotary carousel launcher, tactile rotary encoder wheel navigation, center knob click/hold gesture control, games, sensors, and hardware telemetry.

🔗 **GitHub Repository:** [https://github.com/yhngong/t_embed_multi_app](https://github.com/yhngong/t_embed_multi_app)

---

## 🎛️ Rotary Carousel Launcher
- **Cylindrical Cover-Flow UI:** Beautifully rendered 3D app cards that slide smoothly with rotational inertia.
- **Top Status Bar:** Shows local **San Francisco Bay Area (Pacific Time)**, battery pill indicator with live percentage, and device header.
- **Tactile Dial Controls:**
  - **Turn Rotary Wheel:** Smoothly scroll between apps with interactive card focus animations.
  - **Click Knob:** Launch the selected app with instant visual and LED flash feedback.
  - **Hold Knob (≥ 500ms):** Universal **HOME gesture** from **any app** back to the Carousel Launcher!
- **Onboard APA102 DotStar RGB Halo (7 LEDs):**
  - Chases in real-time along the front dial following rotary wheel turns.
  - Matches the signature theme color of the currently focused app.

---

## 🕹️ Included Apps

### 1. Flappy Bird (Arcade)
- **Controls:**
  - **Click Knob:** Flap wings!
  - **Turn Wheel Clockwise (CW):** Rapid flick-to-flap!
  - **Hold Knob:** Exit back to Launcher.
- **Graphics:** Parallax sky, scrolling clouds, mountain ridges, and striped ground rendered at 50 FPS via LovyanGFX.
- **Difficulty & Score:** Procedural pipes with dynamic gaps and persistent High Score in ESP32 NVS.
- **LED FX:** Amber flying pulse, emerald green flash on scoring, crimson flash on crash.

### 2. Galactic Defender (Space Shooter)
- **Analog Spinner Gameplay:** Turning the rotary wheel directly controls the starfighter's vertical position with sub-pixel analog paddle precision (like classic arcade spinners in *Tempest* and *Breakout*)!
- **Twin Plasma Lasers:** Click knob or hold for rapid auto-fire cannon bursts.
- **Enemies & Asteroids:** Fast scout drones, heavy armored warships, and rotating asteroid fields with particle explosion bursts.
- **Energy Shields:** 3-point shield generator and persistent Best Score tracker.
- **LED FX:** Blue plasma thruster glow; flashes red upon shield damage.

### 3. Retro Snake
- **Rotary Dial Steering:**
  - **Turn CW (Clockwise):** Turn 90° Clockwise (Right relative to current heading).
  - **Turn CCW (Counter-Clockwise):** Turn 90° Counter-Clockwise (Left relative to current heading).
  - **Hold Knob:** Boost crawl speed / Return to Home Menu.
- **Mechanics:** Red apples (+10 pts) and golden bonus stars (+35 pts) with progressive speed scaling.
- **LED FX:** Neon green snake body trail; flashes gold on eating bonus stars.

### 4. CRT Oscilloscope & Signal Analyzer
- **Real-Time Signal Scope:** Green phosphor CRT grid with reticle, voltage divisions, and live waveform capture.
- **Signal Channels:**
  - **CH1:** Battery Voltage ADC (GPIO 4) with real-time mV readout.
  - **CH2:** ESP32-S3 Internal Die Temperature (°C and °F).
  - **CH3:** Synthetic Harmonics Waveform (multi-tone sine generator).
- **Controls:**
  - **Turn Wheel:** Adjust timebase sweep speed (2ms, 5ms, 10ms, 25ms, 50ms, 100ms per division).
  - **Click Knob:** Switch channels.
  - **LED VU Meter:** 7-segment real-time signal peak VU meter!

### 5. World Clock & Timezone Dial
- **Default Timezone:** Configured for the **San Francisco Bay Area / Pacific Time** (`PST8PDT,M3.2.0,M11.1.0`) with automatic Daylight Saving Time.
- **Timezone Rotary Dial:** Rotate the wheel to dial through live world cities:
  - San Francisco (PT) • New York (ET) • London (GMT) • Paris (CET) • Tokyo (JST) • Sydney (AEST) • Honolulu (HST)
- **Dual Display:** Working analog clock face with hour/minute/second hands alongside a large digital clock & calendar.
- **Wi-Fi NTP Sync:** Click knob to trigger Wi-Fi network connection and atomic time synchronization via `pool.ntp.org`.

### 6. Hardware Telemetry & Wi-Fi Scanner
- **SoC Metrics:** ESP32-S3 @ 240MHz, 16MB Flash, 8MB Octal PSRAM, Free SRAM / Heap.
- **Power:** Live battery ADC voltage (GPIO 4) and charge state.
- **Encoder Diagnostics:** Raw tick counter, rotation direction, and knob state.
- **Wi-Fi Scanner:** Scans 2.4GHz bands and lists surrounding networks with SSID, RSSI dBm signal bars, and channels.
- **Controls:** Turn wheel to scroll through telemetry list; click knob to refresh Wi-Fi scan.

---

## 🛠️ Hardware Specifications
- **Board:** LilyGO T-Embed (ESP32-S3-WROOM-1, 16MB Quad SPI Flash, 8MB Octal PSRAM)
- **Display:** 1.9" ST7789V SPI TFT LCD (170×320 native / 320×170 landscape)
  - `SPI2_HOST`, 40MHz write frequency
  - Pin SCLK: `12`, MOSI: `11`, DC: `13`, CS: `10`, RST: `9`, Backlight: `15`
- **Power Enable:** `GPIO 46` (`HIGH` enables power rails)
- **Rotary Encoder Wheel:**
  - Phase A: `GPIO 2`
  - Phase B: `GPIO 1`
  - Center Push Button: `GPIO 0` (Active LOW)
- **RGB LEDs:** 7× APA102 DotStar LEDs (DI: `GPIO 42`, CLK: `GPIO 45`)
- **Battery ADC:** `GPIO 4`
- **Touch Screen:** Removed / not present (100% optimized for rotary wheel and tactile knob clicks).

---

## 🚀 Build & Flash Instructions

### Prerequisites
- [Arduino CLI](https://arduino.github.io/arduino-cli/)
- ESP32 Core (`esp32:esp32`)
- Libraries: `LovyanGFX`, `LilyGo-display-library`, `RotaryEncoder`

### Compile
```bash
arduino-cli compile --fqbn esp32:esp32:lilygo_t_display_s3 .
```

### Upload
```bash
arduino-cli upload -p /dev/cu.usbmodem14501 --fqbn esp32:esp32:lilygo_t_display_s3 .
```
