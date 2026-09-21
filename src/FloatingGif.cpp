#include "FloatingGif.h"

static TFT_eSprite* s_sprite = nullptr;

FloatingGif::FloatingGif(DisplayManager& displayManager)
    : display(displayManager),
      sprite(&displayManager.getTft()),
      isLoaded(false),
      gifWidth(0), gifHeight(0),
      x(100), y(100), prevX(100), prevY(100),
      vx(1.5), vy(1.2),
      boundMinX(0), boundMinY(35), boundMaxX(480), boundMaxY(275),
      lastMoveTime(0), lastFrameTime(0), frameDelayMs(100) {
}

FloatingGif::~FloatingGif() {
    stop();
}

bool FloatingGif::load(const uint8_t* pGifData, size_t size, float startX, float startY, float speedX, float speedY) {
    stop(); // Clean up if previously loaded

    gif.begin(LITTLE_ENDIAN_PIXELS);
    if (!gif.open((uint8_t*)pGifData, size, GIFDrawCallback)) {
        Serial.println("[FloatingGif] Error: Unable to parse GIF data!");
        return false;
    }

    gifWidth = gif.getCanvasWidth();
    gifHeight = gif.getCanvasHeight();

    if (gifWidth == 0 || gifHeight == 0 || gifWidth > 240 || gifHeight > 240) {
        Serial.printf("[FloatingGif] Invalid dimensions: %dx%d (max 240x240 recommended for floating sprite)\n", gifWidth, gifHeight);
        gif.close();
        return false;
    }

    // Allocate small in-memory sprite buffer for zero-flicker rendering
    sprite.setColorDepth(16);
    if (!sprite.createSprite(gifWidth, gifHeight)) {
        Serial.println("[FloatingGif] Error: Out of memory creating sprite!");
        gif.close();
        return false;
    }

    x = startX;
    y = startY;
    prevX = (int16_t)x;
    prevY = (int16_t)y;
    vx = speedX;
    vy = speedY;

    lastMoveTime = millis();
    lastFrameTime = millis();
    isLoaded = true;

    // Decode initial frame into sprite
    decodeNextFrame();

    Serial.printf("[FloatingGif] Loaded %dx%d GIF. Sprite RAM: %u bytes\n",
                  gifWidth, gifHeight, gifWidth * gifHeight * 2);
    return true;
}

void FloatingGif::setBounds(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY) {
    boundMinX = minX;
    boundMinY = minY;
    boundMaxX = maxX;
    boundMaxY = maxY;
}

void FloatingGif::decodeNextFrame() {
    if (!isLoaded) return;

    s_sprite = &sprite;
    sprite.fillSprite(TFT_BLACK); // Clear sprite buffer with key color

    int delayMs = 100;
    int result = gif.playFrame(false, &delayMs);

    if (result == 0) { // End of loop
        gif.reset();
    }

    frameDelayMs = (delayMs > 10 && delayMs < 2000) ? delayMs : 100;
}

void FloatingGif::eraseOldPosition() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(prevX, prevY, gifWidth, gifHeight, TFT_BLACK);
}

void FloatingGif::update() {
    if (!isLoaded) return;

    uint32_t now = millis();

    // 1. Advance GIF frame if frame delay elapsed
    if (now - lastFrameTime >= (uint32_t)frameDelayMs) {
        lastFrameTime = now;
        decodeNextFrame();
    }

    // 2. Advance movement tick (~40 FPS)
    if (now - lastMoveTime >= 25) {
        lastMoveTime = now;

        // Erase old bounding box
        eraseOldPosition();

        // Step coordinates
        x += vx;
        y += vy;

        // Bounce on boundary collisions
        if (x <= boundMinX) {
            x = boundMinX;
            vx = -vx;
        } else if (x + gifWidth >= boundMaxX) {
            x = boundMaxX - gifWidth;
            vx = -vx;
        }

        if (y <= boundMinY) {
            y = boundMinY;
            vy = -vy;
        } else if (y + gifHeight >= boundMaxY) {
            y = boundMaxY - gifHeight;
            vy = -vy;
        }

        prevX = (int16_t)x;
        prevY = (int16_t)y;

        // Push sprite to display with TFT_BLACK as transparent color key
        sprite.pushSprite(prevX, prevY, TFT_BLACK);
    }
}

void FloatingGif::stop() {
    if (isLoaded) {
        eraseOldPosition();
        gif.close();
        sprite.deleteSprite();
        isLoaded = false;
    }
}

void FloatingGif::GIFDrawCallback(GIFDRAW *pDraw) {
    if (!s_sprite) return;

    uint8_t *s;
    uint16_t *usPalette;
    static uint16_t lineBuffer[480];

    int iWidth = pDraw->iWidth;
    int posX = pDraw->iX;
    int posY = pDraw->iY + pDraw->y;

    if (posX + iWidth > s_sprite->width()) iWidth = s_sprite->width() - posX;
    if (posY >= s_sprite->height() || posX >= s_sprite->width() || iWidth <= 0) return;

    usPalette = pDraw->pPalette;
    s = pDraw->pPixels;

    if (pDraw->ucDisposalMethod == 2) {
        for (int x = 0; x < iWidth; x++) {
            if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }

    if (pDraw->ucHasTransparency) {
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        int runLen = 0;
        for (int x = 0; x < iWidth; x++) {
            c = *s++;
            if (c == ucTransparent) {
                if (runLen > 0) {
                    s_sprite->pushImage(posX + x - runLen, posY, runLen, 1, lineBuffer);
                    runLen = 0;
                }
            } else {
                lineBuffer[runLen++] = usPalette[c];
            }
        }
        if (runLen > 0) {
            s_sprite->pushImage(posX + iWidth - runLen, posY, runLen, 1, lineBuffer);
        }
    } else {
        for (int x = 0; x < iWidth; x++) {
            lineBuffer[x] = usPalette[*s++];
        }
        s_sprite->pushImage(posX, posY, iWidth, 1, lineBuffer);
    }
}
