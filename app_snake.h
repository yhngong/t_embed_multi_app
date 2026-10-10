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
  int dir; // 0: Up, 1: Right, 2: Down, 3: Left
  Point food;
  Point bonusStar;
  uint32_t bonusStarExpire;
  int score;
  int highScore;
  bool isGameOver;
  uint32_t lastStepTime;
  Preferences prefs;

  void spawnFood() {
    bool ok = false;
    while (!ok) {
      food.x = random(0, GRID_W);
      food.y = random(0, GRID_H);
      ok = true;
      for (int i = 0; i < length; i++) {
        if (body[i].x == food.x && body[i].y == food.y) {
          ok = false;
          break;
        }
      }
    }
  }

  void spawnBonusStar() {
    bonusStar.x = random(0, GRID_W);
    bonusStar.y = random(0, GRID_H);
    bonusStarExpire = millis() + 7000;
  }

  void resetGame() {
    length = 4;
    dir = 1; // Start moving right
    body[0] = { 8, 7 };
    body[1] = { 7, 7 };
    body[2] = { 6, 7 };
    body[3] = { 5, 7 };
    score = 0;
    isGameOver = false;
    lastStepTime = millis();
    bonusStarExpire = 0;
    spawnFood();
  }

public:
  AppSnake() : length(4), dir(1), score(0), highScore(0), isGameOver(false), lastStepTime(0), bonusStarExpire(0) {}

  void begin() {
    prefs.begin("snake_te", false);
    highScore = prefs.getInt("hiscore", 0);
    resetGame();
    ledBar.setAll(0x003311);
    ledBar.show();
  }

  bool update() {
    if (inputs.wasLongPressed()) {
      prefs.putInt("hiscore", highScore);
      prefs.end();
      return false; // Exit to launcher
    }

    if (isGameOver) {
      if (inputs.wasClicked()) {
        resetGame();
      }
      return true;
    }

    // Rotary Wheel Steering: CW = Turn Right (+1), CCW = Turn Left (-1)
    int delta = inputs.getDelta();
    if (delta > 0) {
      dir = (dir + 1) % 4; // Turn 90 deg clockwise
      ledBar.flash(0x00FF66, 5);
    } else if (delta < 0) {
      dir = (dir + 3) % 4; // Turn 90 deg counter-clockwise
      ledBar.flash(0x00FF66, 5);
    }

    // Speed calculation: speeds up slightly as snake grows
    uint32_t stepInterval = max(60, 150 - (length * 2));
    if (inputs.isKeyDown()) stepInterval /= 2; // Knob held boosts speed!

    if (millis() - lastStepTime >= stepInterval) {
      lastStepTime = millis();

      // Next head position
      Point nextHead = body[0];
      if (dir == 0) nextHead.y--;
      else if (dir == 1) nextHead.x++;
      else if (dir == 2) nextHead.y++;
      else if (dir == 3) nextHead.x--;

      // Wall collision
      if (nextHead.x < 0 || nextHead.x >= GRID_W || nextHead.y < 0 || nextHead.y >= GRID_H) {
        isGameOver = true;
        ledBar.flash(0xFF0000, 16);
        if (score > highScore) {
          highScore = score;
          prefs.putInt("hiscore", highScore);
        }
        return true;
      }

      // Self collision
      for (int i = 0; i < length - 1; i++) {
        if (body[i].x == nextHead.x && body[i].y == nextHead.y) {
          isGameOver = true;
          ledBar.flash(0xFF0000, 16);
          if (score > highScore) {
            highScore = score;
            prefs.putInt("hiscore", highScore);
          }
          return true;
        }
      }

      // Check eating food
      bool ateFood = (nextHead.x == food.x && nextHead.y == food.y);
      bool ateBonus = (bonusStarExpire > millis() && nextHead.x == bonusStar.x && nextHead.y == bonusStar.y);

      // Move body forward
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

        // 25% chance to spawn bonus star
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
        if (dir == 1) { eyeX1 = bx + 6; eyeX2 = bx + 6; eyeY1 = by + 2; eyeY2 = by + 6; }
        else if (dir == 2) { eyeY1 = by + 6; eyeY2 = by + 6; }
        else if (dir == 3) { eyeX1 = bx + 2; eyeX2 = bx + 2; eyeY1 = by + 2; eyeY2 = by + 6; }
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

    // Game Over Overlay
    if (isGameOver) {
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
