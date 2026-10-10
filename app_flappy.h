#pragma once
#include <LilyGo_LovyanGFX.h>
#include <Preferences.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct Pipe {
  float x;
  int gapY;
  bool scored;
};

class AppFlappy {
private:
  float birdY;
  float birdVy;
  int score;
  int highScore;
  bool isDead;
  bool started;
  Preferences prefs;

  static const int NUM_PIPES = 2;
  Pipe pipes[NUM_PIPES];
  float groundScroll;
  float cloudScroll;
  int flapAnimFrame;

  void resetGame() {
    birdY = 70.0f;
    birdVy = 0.0f;
    score = 0;
    isDead = false;
    started = false;
    groundScroll = 0.0f;
    cloudScroll = 0.0f;

    for (int i = 0; i < NUM_PIPES; i++) {
      pipes[i].x = 320.0f + (i * 180.0f);
      pipes[i].gapY = random(35, 95);
      pipes[i].scored = false;
    }
  }

  void flap() {
    birdVy = -3.8f;
    flapAnimFrame = 6;
    ledBar.flash(0xFFAA00, 8);
  }

public:
  AppFlappy() : birdY(70), birdVy(0), score(0), highScore(0), isDead(false), started(false), groundScroll(0), cloudScroll(0), flapAnimFrame(0) {}

  void begin() {
    prefs.begin("flappy_te", false);
    highScore = prefs.getInt("hiscore", 0);
    resetGame();
    ledBar.setAll(0x331100);
    ledBar.show();
  }

  bool update() {
    // Check if user requested HOME (Hold Knob)
    if (inputs.wasLongPressed()) {
      prefs.putInt("hiscore", highScore);
      prefs.end();
      return false; // Exit app
    }

    // Input handling: Click knob or rotate CW to flap!
    bool shouldFlap = false;
    if (inputs.wasClicked()) {
      shouldFlap = true;
    }
    if (inputs.getDelta() > 0) {
      shouldFlap = true; // Rotating CW also flaps!
    }

    if (!started) {
      if (shouldFlap) {
        started = true;
        flap();
      }
      return true;
    }

    if (isDead) {
      if (shouldFlap) {
        resetGame();
      }
      return true;
    }

    if (shouldFlap) {
      flap();
    }

    // Physics
    const float GRAVITY = 0.22f;
    birdVy += GRAVITY;
    birdY += birdVy;

    if (flapAnimFrame > 0) flapAnimFrame--;

    // Ground and ceiling collision
    if (birdY < 8) {
      birdY = 8;
      birdVy = 0;
    }
    if (birdY > 132) {
      birdY = 132;
      isDead = true;
      ledBar.flash(0xFF0000, 16);
      if (score > highScore) {
        highScore = score;
        prefs.putInt("hiscore", highScore);
      }
    }

    // Pipes update
    const float PIPE_SPEED = 1.6f;
    const int PIPE_GAP = 52;
    const int PIPE_WIDTH = 32;

    for (int i = 0; i < NUM_PIPES; i++) {
      pipes[i].x -= PIPE_SPEED;

      // Check scoring
      if (!pipes[i].scored && pipes[i].x + PIPE_WIDTH < 50) {
        pipes[i].scored = true;
        score++;
        ledBar.flash(0x00FF00, 12);
        if (score > highScore) {
          highScore = score;
        }
      }

      // Check collision
      if (pipes[i].x < 50 + 12 && pipes[i].x + PIPE_WIDTH > 50 - 12) {
        if (birdY - 9 < pipes[i].gapY || birdY + 9 > pipes[i].gapY + PIPE_GAP) {
          isDead = true;
          ledBar.flash(0xFF0000, 16);
          if (score > highScore) {
            highScore = score;
            prefs.putInt("hiscore", highScore);
          }
        }
      }

      // Recycle pipe
      if (pipes[i].x < -PIPE_WIDTH) {
        pipes[i].x += NUM_PIPES * 180.0f;
        pipes[i].gapY = random(30, 90);
        pipes[i].scored = false;
      }
    }

    // Parallax scrolling
    groundScroll += PIPE_SPEED;
    if (groundScroll >= 16) groundScroll -= 16;
    cloudScroll += 0.4f;
    if (cloudScroll >= 320) cloudScroll -= 320;

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    // Sky background
    canvas->fillScreen(0x3CF7); // Sky cyan-blue

    // Distant Clouds
    for (int cx = -320; cx < 640; cx += 160) {
      int drawX = cx - (int)cloudScroll;
      canvas->fillCircle(drawX + 40, 28, 18, 0xFFFF);
      canvas->fillCircle(drawX + 60, 24, 22, 0xFFFF);
      canvas->fillCircle(drawX + 80, 28, 18, 0xFFFF);
    }

    // Distant Mountain Silhouettes
    for (int mx = 0; mx < 320; mx += 80) {
      canvas->fillTriangle(mx, 142, mx + 40, 95, mx + 80, 142, 0x2472);
    }

    // Pipes
    const int PIPE_GAP = 52;
    const int PIPE_WIDTH = 32;
    for (int i = 0; i < NUM_PIPES; i++) {
      int px = (int)pipes[i].x;
      int gapY = pipes[i].gapY;

      // Top pipe
      canvas->fillRect(px, 0, PIPE_WIDTH, gapY, 0x2DC4);
      canvas->fillRect(px + 3, 0, 4, gapY, 0x7FA6);
      canvas->drawRect(px, 0, PIPE_WIDTH, gapY, 0x0A62);
      canvas->fillRect(px - 3, gapY - 14, PIPE_WIDTH + 6, 14, 0x2DC4);
      canvas->drawRect(px - 3, gapY - 14, PIPE_WIDTH + 6, 14, 0x0A62);

      // Bottom pipe
      canvas->fillRect(px, gapY + PIPE_GAP, PIPE_WIDTH, 142 - (gapY + PIPE_GAP), 0x2DC4);
      canvas->fillRect(px + 3, gapY + PIPE_GAP, 4, 142 - (gapY + PIPE_GAP), 0x7FA6);
      canvas->drawRect(px, gapY + PIPE_GAP, PIPE_WIDTH, 142 - (gapY + PIPE_GAP), 0x0A62);
      canvas->fillRect(px - 3, gapY + PIPE_GAP, PIPE_WIDTH + 6, 14, 0x2DC4);
      canvas->drawRect(px - 3, gapY + PIPE_GAP, PIPE_WIDTH + 6, 14, 0x0A62);
    }

    // Ground
    canvas->fillRect(0, 142, 320, 28, 0x8BE3); // Warm sand
    canvas->fillRect(0, 142, 320, 4, 0x4E22);  // Grass green cap
    for (int gx = -(int)groundScroll; gx < 320; gx += 16) {
      canvas->drawLine(gx, 146, gx + 8, 170, 0x7361);
    }

    // Animated Flappy Bird
    int bx = 50, by = (int)birdY;
    canvas->fillCircle(bx, by, 12, 0xFD00); // Golden body
    canvas->fillCircle(bx + 5, by - 4, 4, 0xFFFF);
    canvas->fillCircle(bx + 6, by - 4, 2, 0x0000); // Eye
    canvas->fillTriangle(bx + 8, by - 1, bx + 16, by + 2, bx + 8, by + 5, 0xF980); // Beak
    // Wing flapping
    int wingY = (flapAnimFrame > 0) ? (by - 6) : (by + 2);
    canvas->fillRoundRect(bx - 10, wingY, 9, 6, 2, 0xFFFF);

    // Header HUD
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0xFFFF, 0x0000);
    canvas->drawString("SCORE: " + String(score), 10, 8, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::top_right);
    canvas->setTextColor(0xFFE0, 0x0000);
    canvas->drawString("BEST: " + String(highScore), 310, 8, &fonts::Font2);

    // Overlays
    if (!started) {
      canvas->fillRoundRect(40, 50, 240, 54, 8, 0x0000);
      canvas->drawRoundRect(40, 50, 240, 54, 8, 0xFD00);
      canvas->setTextDatum(textdatum_t::middle_center);
      canvas->setTextColor(0xFFFF, 0x0000);
      canvas->drawString("PRESS KNOB OR TURN WHEEL", 160, 67, &fonts::Font2);
      canvas->setTextColor(0x7BEF, 0x0000);
      canvas->drawString("Flap to Fly  *  Hold Knob: Exit", 160, 89, &fonts::Font0);
    } else if (isDead) {
      canvas->fillRoundRect(50, 44, 220, 68, 8, 0x0000);
      canvas->drawRoundRect(50, 44, 220, 68, 8, 0xF800);
      canvas->setTextDatum(textdatum_t::middle_center);
      canvas->setTextColor(0xF800, 0x0000);
      canvas->drawString("GAME OVER", 160, 60, &fonts::Font4);
      canvas->setTextColor(0xFFFF, 0x0000);
      canvas->drawString("Score: " + String(score) + "   Best: " + String(highScore), 160, 84, &fonts::Font2);
      canvas->setTextColor(0x7BEF, 0x0000);
      canvas->drawString("Click: Play Again  *  Hold: Menu", 160, 100, &fonts::Font0);
    }
  }
};
