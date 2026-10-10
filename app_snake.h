#pragma once
#include <LilyGo_LovyanGFX.h>
#include <Preferences.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct Point {
  int8_t x, y;
};

class AppSnake {
private:
  static const int GRID_W = 32;
  static const int GRID_H = 15;
  static const int CELL_SIZE = 9;
  static const int OFFSET_X = 16;
  static const int OFFSET_Y = 24;

  Point body[128];
  int length;
  int currentDir; // 0: Up, 1: Right, 2: Down, 3: Left
  int queuedDir;  // Direction buffered for next step
  bool turnBuffered;

  Point food;
  Point bonusStar;
  uint32_t bonusStarExpire;
  int score;
  int highScore;
  bool started;
  bool isGameOver;
  uint32_t gameOverTime;
  uint32_t lastStepTime;
  Preferences prefs;

  void spawnFood() {
    bool ok = false;
    int attempts = 0;
    while (!ok && attempts < 200) {
      food.x = random(1, GRID_W - 1);
      food.y = random(1, GRID_H - 1);
      ok = true;
      for (int i = 0; i < length; i++) {
        if (body[i].x == food.x && body[i].y == food.y) {
          ok = false;
          break;
        }
      }
      attempts++;
    }
  }

  void spawnBonusStar() {
    bonusStar.x = random(1, GRID_W - 1);
    bonusStar.y = random(1, GRID_H - 1);
    bonusStarExpire = millis() + 8000;
  }

  void resetGame() {
    length = 4;
    currentDir = 1; // Moving Right
    queuedDir = 1;
    turnBuffered = false;

    // Center snake safely away from walls
    body[0] = { 10, 7 };
    body[1] = { 9, 7 };
    body[2] = { 8, 7 };
    body[3] = { 7, 7 };

    score = 0;
    started = false;
    isGameOver = false;
    gameOverTime = 0;
    lastStepTime = millis();
    bonusStarExpire = 0;
    spawnFood();
  }

  // Prevent 180-degree reversals
  bool isOpposite(int d1, int d2) {
    return (d1 == 0 && d2 == 2) || (d1 == 2 && d2 == 0) ||
           (d1 == 1 && d2 == 3) || (d1 == 3 && d2 == 1);
  }

public:
  AppSnake() : length(4), currentDir(1), queuedDir(1), turnBuffered(false), score(0), highScore(0), started(false), isGameOver(false), gameOverTime(0), lastStepTime(0), bonusStarExpire(0) {}

  void begin() {
    prefs.begin("snake_te", false);
    highScore = prefs.getInt("hiscore", 0);
    inputs.clearPending();
    resetGame();
    ledBar.setAll(0x003311);
    ledBar.show();
  }

  bool update() {
    // Universal HOME gesture (Hold Knob)
    if (inputs.wasLongPressed()) {
      prefs.putInt("hiscore", highScore);
      prefs.end();
      return false; // Exit to launcher
    }

    // Waiting to start game
    if (!started) {
      if (inputs.wasClicked()) {
        started = true;
        lastStepTime = millis();
        ledBar.flash(0x00FF66, 8);
      }
      return true;
    }

    // Game Over state with debounced restart
    if (isGameOver) {
      if (inputs.wasClicked() && (millis() - gameOverTime > 400)) {
        resetGame();
      }
      return true;
    }

    // Rotary Wheel Steering:
    // Buffer only ONE 90-degree turn per game step to prevent 180-degree self-reversals
    int delta = inputs.getDelta();
    if (delta != 0 && !turnBuffered) {
      int desiredDir = currentDir;
      if (delta > 0) {
        desiredDir = (currentDir + 1) % 4; // 90° Clockwise
      } else {
        desiredDir = (currentDir + 3) % 4; // 90° Counter-Clockwise
      }

      if (!isOpposite(desiredDir, currentDir)) {
        queuedDir = desiredDir;
        turnBuffered = true;
        ledBar.flash(0x00FF66, 4);
      }
    }

    // Speed calculation
    uint32_t stepInterval = max(80, 160 - (length * 2));
    if (inputs.isKeyDown()) stepInterval /= 2; // Hold knob to sprint

    if (millis() - lastStepTime >= stepInterval) {
      lastStepTime = millis();

      // Apply buffered turn
      currentDir = queuedDir;
      turnBuffered = false; // Ready for next turn

      // Next head position
      Point nextHead = body[0];
      if (currentDir == 0) nextHead.y--;
      else if (currentDir == 1) nextHead.x++;
      else if (currentDir == 2) nextHead.y++;
      else if (currentDir == 3) nextHead.x--;

      // Wall collision
      if (nextHead.x < 0 || nextHead.x >= GRID_W || nextHead.y < 0 || nextHead.y >= GRID_H) {
        isGameOver = true;
        gameOverTime = millis();
        ledBar.flash(0xFF0000, 16);
        if (score > highScore) {
          highScore = score;
          prefs.putInt("hiscore", highScore);
        }
        return true;
      }

      // Self collision: check body segments from neck (1) to tail (length - 1)
      for (int i = 1; i < length; i++) {
        if (body[i].x == nextHead.x && body[i].y == nextHead.y) {
          isGameOver = true;
          gameOverTime = millis();
          ledBar.flash(0xFF0000, 16);
          if (score > highScore) {
            highScore = score;
            prefs.putInt("hiscore", highScore);
          }
          return true;
        }
      }

      // Check eating food / bonus
      bool ateFood = (nextHead.x == food.x && nextHead.y == food.y);
      bool ateBonus = (bonusStarExpire > millis() && nextHead.x == bonusStar.x && nextHead.y == bonusStar.y);

      // Move body segments forward
      if (ateFood || ateBonus) {
        if (length < 127) length++;
      }
      for (int i = length - 1; i > 0; i--) {
        body[i] = body[i - 1];
      }
      body[0] = nextHead;

      if (ateFood) {
        score += 10;
        spawnFood();
        ledBar.flash(0x00FF00, 10);
        if (score > highScore) highScore = score;

        if (random(0, 4) == 0 && bonusStarExpire < millis()) {
          spawnBonusStar();
        }
      } else if (ateBonus) {
        score += 35;
        bonusStarExpire = 0;
        ledBar.flash(0xFFFF00, 14);
        if (score > highScore) highScore = score;
      }
    }

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    canvas->fillScreen(0x0821);

    // Top HUD Bar
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0x07E0, 0x0821);
    canvas->drawString("SNAKE", 14, 6, &fonts::Font2);

    canvas->setTextColor(0xFFFF, 0x0821);
    canvas->drawString("SCORE: " + String(score), 90, 6, &fonts::Font2);

    canvas->setTextDatum(textdatum_t::top_right);
    canvas->setTextColor(0xFD00, 0x0821);
    canvas->drawString("BEST: " + String(highScore), 306, 6, &fonts::Font2);

    // Game Arena Border
    canvas->drawRoundRect(OFFSET_X - 2, OFFSET_Y - 2, (GRID_W * CELL_SIZE) + 4, (GRID_H * CELL_SIZE) + 4, 4, 0x2145);
    canvas->fillRect(OFFSET_X, OFFSET_Y, GRID_W * CELL_SIZE, GRID_H * CELL_SIZE, 0x0000);

    // Draw Food (Apple)
    int fx = OFFSET_X + food.x * CELL_SIZE + 4;
    int fy = OFFSET_Y + food.y * CELL_SIZE + 4;
    canvas->fillCircle(fx, fy, 4, 0xF800);
    canvas->drawPixel(fx + 1, fy - 4, 0x07E0); // Leaf

    // Draw Bonus Star if active
    if (bonusStarExpire > millis()) {
      int sx = OFFSET_X + bonusStar.x * CELL_SIZE + 4;
      int sy = OFFSET_Y + bonusStar.y * CELL_SIZE + 4;
      canvas->fillCircle(sx, sy, 4, 0xFFE0);
      canvas->drawCircle(sx, sy, 5, 0xFFAA);
    }

    // Draw Snake Body
    for (int i = 0; i < length; i++) {
      int bx = OFFSET_X + body[i].x * CELL_SIZE;
      int by = OFFSET_Y + body[i].y * CELL_SIZE;
      if (i == 0) {
        // Snake Head with eyes
        canvas->fillRoundRect(bx, by, CELL_SIZE, CELL_SIZE, 3, 0x07E0);
        int eyeX1 = bx + 2, eyeY1 = by + 2;
        int eyeX2 = bx + 6, eyeY2 = by + 2;
        if (currentDir == 1) { eyeX1 = bx + 6; eyeX2 = bx + 6; eyeY1 = by + 2; eyeY2 = by + 6; }
        else if (currentDir == 2) { eyeY1 = by + 6; eyeY2 = by + 6; }
        else if (currentDir == 3) { eyeX1 = bx + 2; eyeX2 = bx + 2; eyeY1 = by + 2; eyeY2 = by + 6; }
        canvas->drawPixel(eyeX1, eyeY1, 0x0000);
        canvas->drawPixel(eyeX2, eyeY2, 0x0000);
      } else {
        // Snake body segments
        uint16_t bCol = (i % 2 == 0) ? 0x0660 : 0x0540;
        canvas->fillRoundRect(bx + 1, by + 1, CELL_SIZE - 2, CELL_SIZE - 2, 2, bCol);
      }
    }

    // Controls hint
    canvas->setTextDatum(textdatum_t::bottom_center);
    canvas->setTextColor(0x7BEF, 0x0821);
    canvas->drawString("Turn Wheel: Rotate Head   *   Hold Knob: Home", 160, 168, &fonts::Font0);

    // Ready / Start Overlay
    if (!started) {
      canvas->fillRoundRect(48, 48, 224, 66, 8, 0x0000);
      canvas->drawRoundRect(48, 48, 224, 66, 8, 0x07E0);
      canvas->setTextDatum(textdatum_t::middle_center);
      canvas->setTextColor(0x07E0, 0x0000);
      canvas->drawString("RETRO SNAKE", 160, 64, &fonts::Font4);
      canvas->setTextColor(0xFFFF, 0x0000);
      canvas->drawString("PRESS KNOB TO START", 160, 88, &fonts::Font2);
      canvas->setTextColor(0x7BEF, 0x0000);
      canvas->drawString("Turn Wheel: CW/CCW Turn", 160, 102, &fonts::Font0);
    } else if (isGameOver) {
      canvas->fillRoundRect(50, 48, 220, 68, 8, 0x0000);
      canvas->drawRoundRect(50, 48, 220, 68, 8, 0xF800);
      canvas->setTextDatum(textdatum_t::middle_center);
      canvas->setTextColor(0xF800, 0x0000);
      canvas->drawString("GAME OVER", 160, 64, &fonts::Font4);
      canvas->setTextColor(0xFFFF, 0x0000);
      canvas->drawString("Score: " + String(score) + "   Best: " + String(highScore), 160, 88, &fonts::Font2);
      canvas->setTextColor(0x7BEF, 0x0000);
      canvas->drawString("Click: Play Again  *  Hold: Menu", 160, 104, &fonts::Font0);
    }
  }
};
