#pragma once
#include <LilyGo_LovyanGFX.h>
#include <Preferences.h>
#include "config.h"
#include "input.h"
#include "apa102_led.h"

struct Bullet {
  float x, y;
  bool active;
};

struct Enemy {
  float x, y;
  float vx;
  int type; // 0 = scout drone, 1 = heavy cruiser, 2 = asteroid
  int hp;
  bool active;
};

struct Particle {
  float x, y;
  float vx, vy;
  uint16_t color;
  int life;
};

class AppShooter {
private:
  float playerY;
  float targetY;
  int shields;
  int score;
  int highScore;
  bool gameOver;
  Preferences prefs;

  static const int MAX_BULLETS = 6;
  Bullet bullets[MAX_BULLETS];

  static const int MAX_ENEMIES = 5;
  Enemy enemies[MAX_ENEMIES];

  static const int MAX_PARTICLES = 24;
  Particle particles[MAX_PARTICLES];

  uint32_t lastShootTime;
  uint32_t lastSpawnTime;
  float starfieldScroll[3];

  void spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) {
        enemies[i].active = true;
        enemies[i].x = 325.0f;
        enemies[i].y = random(20, 150);
        enemies[i].type = random(0, 3);
        if (enemies[i].type == 0) {
          enemies[i].vx = random(24, 38) / 10.0f; // fast scout
          enemies[i].hp = 1;
        } else if (enemies[i].type == 1) {
          enemies[i].vx = random(12, 20) / 10.0f; // heavy cruiser
          enemies[i].hp = 3;
        } else {
          enemies[i].vx = random(16, 26) / 10.0f; // asteroid
          enemies[i].hp = 2;
        }
        break;
      }
    }
  }

  void spawnExplosion(float x, float y, uint16_t color, int count = 8) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
      if (particles[i].life <= 0) {
        particles[i].x = x;
        particles[i].y = y;
        float angle = random(0, 360) * 0.0174533f;
        float spd = random(10, 35) / 10.0f;
        particles[i].vx = cos(angle) * spd;
        particles[i].vy = sin(angle) * spd;
        particles[i].color = color;
        particles[i].life = random(10, 20);
        spawned++;
      }
    }
  }

  void fireBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active) {
        bullets[i].active = true;
        bullets[i].x = 38.0f;
        bullets[i].y = playerY;
        lastShootTime = millis();
        ledBar.flash(0x00D9FF, 6);
        break;
      }
    }
  }

  void resetGame() {
    playerY = 85.0f;
    targetY = 85.0f;
    shields = 3;
    score = 0;
    gameOver = false;
    lastShootTime = 0;
    lastSpawnTime = millis();

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0;
  }

public:
  AppShooter() : playerY(85), targetY(85), shields(3), score(0), highScore(0), gameOver(false), lastShootTime(0), lastSpawnTime(0) {
    starfieldScroll[0] = starfieldScroll[1] = starfieldScroll[2] = 0;
  }

  void begin() {
    prefs.begin("shooter_te", false);
    highScore = prefs.getInt("hiscore", 0);
    resetGame();
    ledBar.setAll(0x001133);
    ledBar.show();
  }

  bool update() {
    if (inputs.wasLongPressed()) {
      prefs.putInt("hiscore", highScore);
      prefs.end();
      return false; // Exit to launcher
    }

    if (gameOver) {
      if (inputs.wasClicked()) {
        resetGame();
      }
      return true;
    }

    // Rotary Wheel Controls Y Axis!
    int delta = inputs.getDelta();
    if (delta != 0) {
      targetY += delta * 7.0f; // smooth paddle response
      targetY = constrain(targetY, 18.0f, 152.0f);
    }
    // Smooth lerp to target position
    playerY += (targetY - playerY) * 0.45f;

    // Fire laser on knob click or auto-burst while held
    if (inputs.wasClicked() || (inputs.isKeyDown() && (millis() - lastShootTime >= 240))) {
      fireBullet();
    }

    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullets[i].active) {
        bullets[i].x += 7.5f;
        if (bullets[i].x > 320) bullets[i].active = false;
      }
    }

    // Spawn enemies
    if (millis() - lastSpawnTime >= 1200) {
      spawnEnemy();
      lastSpawnTime = millis();
    }

    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].active) {
        enemies[i].x -= enemies[i].vx;

        // Collision with bullets
        for (int b = 0; b < MAX_BULLETS; b++) {
          if (bullets[b].active) {
            float dx = bullets[b].x - enemies[i].x;
            float dy = bullets[b].y - enemies[i].y;
            if (abs(dx) < 14 && abs(dy) < 12) {
              bullets[b].active = false;
              enemies[i].hp--;
              spawnExplosion(bullets[b].x, bullets[b].y, 0xFFE0, 4);

              if (enemies[i].hp <= 0) {
                enemies[i].active = false;
                score += (enemies[i].type == 1) ? 35 : 15;
                spawnExplosion(enemies[i].x, enemies[i].y, 0xF980, 10);
                if (score > highScore) highScore = score;
              }
              break;
            }
          }
        }

        // Collision with player ship
        if (enemies[i].active) {
          float pdx = enemies[i].x - 24.0f;
          float pdy = enemies[i].y - playerY;
          if (abs(pdx) < 18 && abs(pdy) < 14) {
            enemies[i].active = false;
            shields--;
            spawnExplosion(30, playerY, 0xF800, 14);
            ledBar.flash(0xFF0000, 16);

            if (shields <= 0) {
              gameOver = true;
              if (score > highScore) {
                highScore = score;
                prefs.putInt("hiscore", highScore);
              }
            }
          }
        }

        // Offscreen check
        if (enemies[i].x < -20) {
          enemies[i].active = false;
        }
      }
    }

    // Update particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (particles[i].life > 0) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].life--;
      }
    }

    // Starfield parallax
    starfieldScroll[0] += 0.8f;
    starfieldScroll[1] += 2.2f;
    starfieldScroll[2] += 4.5f;

    return true;
  }

  void draw(LGFX_Sprite* canvas) {
    canvas->fillScreen(0x0002); // Deep Cosmos

    // Parallax Starfields
    for (int s = 0; s < 3; s++) {
      uint16_t starColor = (s == 0) ? 0x3186 : ((s == 1) ? 0x738E : 0xFFFF);
      for (int i = 0; i < 18; i++) {
        int sx = (int)(i * 47 - starfieldScroll[s]) % 320;
        if (sx < 0) sx += 320;
        int sy = (i * 29 + s * 13) % 170;
        canvas->drawPixel(sx, sy, starColor);
      }
    }

    // Draw Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (particles[i].life > 0) {
        canvas->fillCircle((int)particles[i].x, (int)particles[i].y, 1, particles[i].color);
      }
    }

    // Draw Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullets[i].active) {
        int bx = (int)bullets[i].x;
        int by = (int)bullets[i].y;
        canvas->fillRect(bx, by - 1, 10, 3, 0x07FF);
        canvas->fillRect(bx + 4, by, 4, 1, 0xFFFF);
      }
    }

    // Draw Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].active) {
        int ex = (int)enemies[i].x;
        int ey = (int)enemies[i].y;
        if (enemies[i].type == 0) {
          // Scout Drone (Red/Orange dart)
          canvas->fillTriangle(ex - 10, ey, ex + 8, ey - 7, ex + 8, ey + 7, 0xF800);
          canvas->fillCircle(ex - 2, ey, 2, 0xFFE0);
        } else if (enemies[i].type == 1) {
          // Heavy Cruiser (Purple/Cyan armored warship)
          canvas->fillRoundRect(ex - 14, ey - 8, 24, 16, 4, 0x981F);
          canvas->drawRoundRect(ex - 14, ey - 8, 24, 16, 4, 0x07FF);
          canvas->fillRect(ex - 6, ey - 2, 8, 4, 0xF800);
        } else {
          // Space Asteroid (Chiseled Rock)
          canvas->fillCircle(ex, ey, 9, 0x7BEF);
          canvas->drawCircle(ex, ey, 9, 0x4208);
          canvas->fillCircle(ex - 2, ey - 2, 2, 0x3186);
        }
      }
    }

    // Draw Player Ship (Sleek Interceptor)
    int px = 24, py = (int)playerY;
    // Thruster flame
    int flameLen = random(6, 12);
    canvas->fillTriangle(px - 10 - flameLen, py, px - 10, py - 4, px - 10, py + 4, 0x07FF);
    canvas->fillTriangle(px - 10 - (flameLen / 2), py, px - 10, py - 2, px - 10, py + 2, 0xFFFF);
    // Fuselage
    canvas->fillTriangle(px + 14, py, px - 10, py - 9, px - 10, py + 9, 0x07FF);
    canvas->fillRoundRect(px - 6, py - 3, 12, 6, 2, 0xFFFF);
    canvas->drawLine(px - 10, py - 9, px - 14, py - 12, 0x001F);
    canvas->drawLine(px - 10, py + 9, px - 14, py + 12, 0x001F);

    // Top HUD
    canvas->setTextDatum(textdatum_t::top_left);
    canvas->setTextColor(0xFFFF, 0x0002);
    canvas->drawString("SCORE: " + String(score), 8, 6, &fonts::Font2);

    // Shields Bar
    canvas->setTextDatum(textdatum_t::top_center);
    canvas->setTextColor(0x07FF, 0x0002);
    canvas->drawString("SHIELDS", 160, 6, &fonts::Font0);
    for (int s = 0; s < 3; s++) {
      uint16_t sColor = (s < shields) ? 0x07E0 : 0x3186;
      canvas->fillRoundRect(134 + s * 18, 15, 14, 5, 2, sColor);
    }

    canvas->setTextDatum(textdatum_t::top_right);
    canvas->setTextColor(0xFD00, 0x0002);
    canvas->drawString("BEST: " + String(highScore), 312, 6, &fonts::Font2);

    // Game Over Overlay
    if (gameOver) {
      canvas->fillRoundRect(45, 42, 230, 72, 8, 0x0000);
      canvas->drawRoundRect(45, 42, 230, 72, 8, 0xF800);
      canvas->setTextDatum(textdatum_t::middle_center);
      canvas->setTextColor(0xF800, 0x0000);
      canvas->drawString("CRITICAL FAILURE", 160, 58, &fonts::Font4);
      canvas->setTextColor(0xFFFF, 0x0000);
      canvas->drawString("Score: " + String(score) + "   Best: " + String(highScore), 160, 82, &fonts::Font2);
      canvas->setTextColor(0x7BEF, 0x0000);
      canvas->drawString("Click: Retry  *  Hold: Exit to Menu", 160, 99, &fonts::Font0);
    }
  }
};
