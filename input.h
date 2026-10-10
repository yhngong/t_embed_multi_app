#pragma once
#include <Arduino.h>
#include <RotaryEncoder.h>
#include "config.h"

class InputManager {
private:
  RotaryEncoder encoder;
  int lastPosition;
  int deltaTicks;

  bool btnPhysicalState;
  bool btnLastState;
  uint32_t btnPressStartTime;
  bool longPressTriggered;
  bool clickPending;
  bool longPressPending;

public:
  InputManager() 
    : encoder(PIN_ENCODE_A, PIN_ENCODE_B, RotaryEncoder::LatchMode::TWO03),
      lastPosition(0), deltaTicks(0),
      btnPhysicalState(false), btnLastState(false),
      btnPressStartTime(0), longPressTriggered(false),
      clickPending(false), longPressPending(false) {}

  void begin() {
    pinMode(PIN_ENCODE_A, INPUT_PULLUP);
    pinMode(PIN_ENCODE_B, INPUT_PULLUP);
    pinMode(PIN_ENCODE_BTN, INPUT_PULLUP);
  }

  void tickInterrupt() {
    encoder.tick();
  }

  void update() {
    // Process encoder delta
    int currentPos = encoder.getPosition();
    deltaTicks = currentPos - lastPosition;
    lastPosition = currentPos;

    // Process button
    bool isDown = (digitalRead(PIN_ENCODE_BTN) == LOW);
    uint32_t now = millis();

    if (isDown && !btnLastState) {
      // Just pressed down
      btnPhysicalState = true;
      btnPressStartTime = now;
      longPressTriggered = false;
    } else if (isDown && btnLastState) {
      // Still held down
      if (!longPressTriggered && (now - btnPressStartTime >= 500)) {
        longPressTriggered = true;
        longPressPending = true; // Signal HOME gesture!
      }
    } else if (!isDown && btnLastState) {
      // Just released
      btnPhysicalState = false;
      if (!longPressTriggered && (now - btnPressStartTime >= 30)) {
        // Debounced short click
        clickPending = true;
      }
      longPressTriggered = false;
    }

    btnLastState = isDown;
  }

  int getDelta() const {
    return deltaTicks;
  }

  int getAbsolutePosition() const {
    return lastPosition;
  }

  void setPosition(int p) {
    encoder.setPosition(p);
    lastPosition = p;
    deltaTicks = 0;
  }

  bool isKeyDown() const {
    return btnPhysicalState;
  }

  bool wasClicked() {
    if (clickPending) {
      clickPending = false;
      return true;
    }
    return false;
  }

  bool wasLongPressed() {
    if (longPressPending) {
      longPressPending = false;
      return true;
    }
    return false;
  }

  void clearPending() {
    clickPending = false;
    longPressPending = false;
    deltaTicks = 0;
  }
};

extern InputManager inputs;
