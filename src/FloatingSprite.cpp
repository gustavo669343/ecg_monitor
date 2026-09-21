#include "FloatingSprite.h"

FloatingSprite::FloatingSprite(DisplayManager& displayManager)
    : display(displayManager),
      sprite(&displayManager.getTft()),
      isLoaded(false),
      spriteWidth(0), spriteHeight(0),
      x(200), y(150), prevX(200), prevY(150),
      vx(-1.5), vy(1.3),
      boundMinX(5), boundMinY(38), boundMaxX(475), boundMaxY(275),
      lastMoveTime(0) {
}

FloatingSprite::~FloatingSprite() {
    stop();
}

bool FloatingSprite::loadRGBA(const uint32_t* pRgbaData, uint16_t w, uint16_t h, 
                              float startX, float startY, float speedX, float speedY) {
    stop();

    spriteWidth = w;
    spriteHeight = h;

    sprite.setColorDepth(16);
    if (!sprite.createSprite(w, h)) {
        Serial.println("[FloatingSprite] Error: Out of memory creating sprite!");
        return false;
    }

    // Chuyển đổi mảng RGBA8888 sang RGB565 trong sprite buffer
    sprite.fillSprite(TFT_BLACK); // Nền đen làm key màu trong suốt
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint32_t pixel = pgm_read_dword(&pRgbaData[row * w + col]);
            uint8_t a = pixel & 0xFF; // LSB là Alpha
            if (a >= 128) {
                uint8_t r = (pixel >> 24) & 0xFF;
                uint8_t g = (pixel >> 16) & 0xFF;
                uint8_t b = (pixel >> 8) & 0xFF;
                uint16_t c565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                // Tránh trùng với màu đen trong suốt TFT_BLACK (0x0000)
                if (c565 == TFT_BLACK) c565 = 0x0821;
                sprite.drawPixel(col, row, c565);
            }
        }
    }

    x = startX;
    y = startY;
    prevX = (int16_t)x;
    prevY = (int16_t)y;
    vx = speedX;
    vy = speedY;

    lastMoveTime = millis();
    isLoaded = true;

    Serial.printf("[FloatingSprite] Loaded %dx%d Sprite. RAM: %u bytes\n", w, h, w * h * 2);
    return true;
}

void FloatingSprite::setBounds(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY) {
    boundMinX = minX;
    boundMinY = minY;
    boundMaxX = maxX;
    boundMaxY = maxY;
}

void FloatingSprite::eraseOldPosition() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(prevX, prevY, spriteWidth, spriteHeight, TFT_BLACK);
}

void FloatingSprite::update() {
    if (!isLoaded) return;

    uint32_t now = millis();
    // Chạy ở ~40 FPS (25ms mỗi bước)
    if (now - lastMoveTime >= 25) {
        lastMoveTime = now;

        eraseOldPosition();

        x += vx;
        y += vy;

        if (x <= boundMinX) {
            x = boundMinX;
            vx = -vx;
        } else if (x + spriteWidth >= boundMaxX) {
            x = boundMaxX - spriteWidth;
            vx = -vx;
        }

        if (y <= boundMinY) {
            y = boundMinY;
            vy = -vy;
        } else if (y + spriteHeight >= boundMaxY) {
            y = boundMaxY - spriteHeight;
            vy = -vy;
        }

        prevX = (int16_t)x;
        prevY = (int16_t)y;

        // Đẩy sprite ra màn hình (TFT_BLACK làm màu trong suốt)
        sprite.pushSprite(prevX, prevY, TFT_BLACK);
    }
}

void FloatingSprite::stop() {
    if (isLoaded) {
        eraseOldPosition();
        sprite.deleteSprite();
        isLoaded = false;
    }
}
