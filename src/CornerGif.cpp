#include "CornerGif.h"

static TFT_eSprite* s_cornerSprite = nullptr;
static uint8_t s_cornerScale = 1;

CornerGif::CornerGif(DisplayManager& displayManager)
    : display(displayManager),
      sprite(&displayManager.getTft()),
      isLoaded(false),
      x(278), y(40),
      scaleFactor(6),
      origWidth(0), origHeight(0),
      currentRegistryIndex(-1),
      lastFrameTime(0),
      frameDelayMs(100),
      timeoutActive(false),
      timeoutStart(0),
      timeoutDuration(0) {
    strncpy(timeoutTarget, "normal", sizeof(timeoutTarget));
}

CornerGif::~CornerGif() {
    stop();
}

void CornerGif::eraseArea() {
    if (isLoaded && origWidth > 0 && origHeight > 0) {
        display.getTft().fillRect(x, y, getScaledWidth(), getScaledHeight(), TFT_BLACK);
    }
}

bool CornerGif::load(const uint8_t* pGifData, size_t size, int16_t posX, int16_t posY, uint8_t scale) {
    eraseArea();
    stop();

    scaleFactor = (scale == 0) ? 1 : scale;
    x = posX;
    y = posY;

    gif.begin(LITTLE_ENDIAN_PIXELS);
    if (!gif.open((uint8_t*)pGifData, size, GIFDrawCallback)) {
        Serial.println("[CornerGif] Error: Unable to parse GIF data!");
        return false;
    }

    origWidth = gif.getCanvasWidth();
    origHeight = gif.getCanvasHeight();

    if (origWidth == 0 || origHeight == 0) {
        Serial.println("[CornerGif] Invalid GIF dimensions!");
        gif.close();
        return false;
    }

    uint16_t scaledW = origWidth * scaleFactor;
    uint16_t scaledH = origHeight * scaleFactor;

    sprite.setColorDepth(16);
    if (!sprite.createSprite(scaledW, scaledH)) {
        Serial.printf("[CornerGif] Error: Out of memory creating scaled sprite (%dx%d = %u bytes)!\n",
                      scaledW, scaledH, scaledW * scaledH * 2);
        gif.close();
        return false;
    }

    isLoaded = true;
    lastFrameTime = millis();

    // Decode and display first frame immediately
    decodeNextFrame();

    Serial.printf("[CornerGif] Loaded %dx%d GIF scaled x%d -> %dx%d at (%d, %d). Sprite RAM: %u bytes\n",
                  origWidth, origHeight, scaleFactor, scaledW, scaledH, posX, posY, scaledW * scaledH * 2);
    return true;
}

bool CornerGif::playByName(const char* name) {
    if (!name) return false;
    for (size_t i = 0; i < GIF_REGISTRY_COUNT; i++) {
        if (strcasecmp(GIF_REGISTRY[i].name, name) == 0) {
            return playIndex(i);
        }
    }
    Serial.printf("[CornerGif] Error: GIF named '%s' not found in registry!\n", name);
    return false;
}

bool CornerGif::playIndex(size_t index) {
    if (index >= GIF_REGISTRY_COUNT) return false;

    const GifEntry& entry = GIF_REGISTRY[index];
    currentRegistryIndex = (int)index;

    Serial.printf("[CornerGif] Switching to: [%d] '%s' (Scale x%d)...\n",
                  index, entry.name, entry.defaultScale);

    bool ok = load(entry.data, entry.size, x, y, entry.defaultScale);
    if (ok) {
        if (strcasecmp(entry.name, "petted") == 0) {
            setTimeout(defaultPetTimeoutMs, "normal");
            Serial.printf("[CornerGif] >> Kich hoat hen gio: 'petted' se tu dong timeout sau %u giay -> ve 'normal' <<\n", (unsigned int)(defaultPetTimeoutMs / 1000));
        } else {
            cancelTimeout();
        }
    }
    return ok;
}

void CornerGif::setTimeout(uint32_t durationMs, const char* revertTo) {
    timeoutActive = true;
    timeoutDuration = durationMs;
    timeoutStart = millis();
    if (revertTo && strlen(revertTo) > 0) {
        strncpy(timeoutTarget, revertTo, sizeof(timeoutTarget) - 1);
        timeoutTarget[sizeof(timeoutTarget) - 1] = '\0';
    } else {
        strncpy(timeoutTarget, "normal", sizeof(timeoutTarget) - 1);
    }
}

void CornerGif::refreshTimeout() {
    if (timeoutActive) {
        timeoutStart = millis();
    }
}

void CornerGif::cancelTimeout() {
    timeoutActive = false;
}

void CornerGif::next() {
    if (GIF_REGISTRY_COUNT == 0) return;
    size_t nextIdx = (currentRegistryIndex + 1) % GIF_REGISTRY_COUNT;
    playIndex(nextIdx);
}

void CornerGif::prev() {
    if (GIF_REGISTRY_COUNT == 0) return;
    size_t prevIdx = (currentRegistryIndex - 1 + GIF_REGISTRY_COUNT) % GIF_REGISTRY_COUNT;
    playIndex(prevIdx);
}

const char* CornerGif::getCurrentName() const {
    if (currentRegistryIndex >= 0 && currentRegistryIndex < (int)GIF_REGISTRY_COUNT) {
        return GIF_REGISTRY[currentRegistryIndex].name;
    }
    return "custom";
}

void CornerGif::decodeNextFrame() {
    if (!isLoaded) return;

    s_cornerSprite = &sprite;
    s_cornerScale = scaleFactor;

    sprite.fillSprite(TFT_BLACK); // Nền đen làm key màu trong suốt

    int delayMs = 100;
    int result = gif.playFrame(false, &delayMs);

    if (result == 0) {
        gif.reset();
    }

    frameDelayMs = (delayMs > 10 && delayMs < 2000) ? delayMs : 100;

    // Đẩy sprite ra màn hình ở vị trí cố định x, y (không dùng transparent key để xóa sạch vết khung hình cũ)
    sprite.pushSprite(x, y);
}

void CornerGif::update() {
    if (!isLoaded) return;

    uint32_t now = millis();

    // 1. Kiểm tra timeout biểu cảm (ví dụ 'petted' sau 3 giây quay về 'normal')
    if (timeoutActive && (now - timeoutStart >= timeoutDuration)) {
        timeoutActive = false;
        Serial.printf("[CornerGif] '%s' da het 3 giay (timed out). Tu dong quay ve '%s'!\n",
                      getCurrentName(), timeoutTarget);
        playByName(timeoutTarget);
        return;
    }

    // 2. Cập nhật khung hình hoạt hình GIF
    if (now - lastFrameTime >= (uint32_t)frameDelayMs) {
        lastFrameTime = now;
        decodeNextFrame();
    }
}

void CornerGif::redraw() {
    if (isLoaded) {
        sprite.pushSprite(x, y);
    }
}

void CornerGif::stop() {
    if (isLoaded) {
        gif.close();
        sprite.deleteSprite();
        isLoaded = false;
    }
}

void CornerGif::GIFDrawCallback(GIFDRAW *pDraw) {
    if (!s_cornerSprite) return;

    uint8_t scale = s_cornerScale;
    uint8_t *s = pDraw->pPixels;
    uint16_t *usPalette = pDraw->pPalette;
    int iWidth = pDraw->iWidth;
    int posY = (pDraw->iY + pDraw->y) * scale;
    int posX = pDraw->iX * scale;

    if (pDraw->ucHasTransparency) {
        uint8_t ucTransparent = pDraw->ucTransparent;
        for (int x = 0; x < iWidth; x++) {
            uint8_t c = s[x];
            if (c != ucTransparent) {
                uint16_t col = usPalette[c];
                if (col == TFT_BLACK) col = 0x0821; // Tránh trùng màu trong suốt
                s_cornerSprite->fillRect(posX + x * scale, posY, scale, scale, col);
            }
        }
    } else {
        for (int x = 0; x < iWidth; x++) {
            uint16_t col = usPalette[s[x]];
            if (col == TFT_BLACK) col = 0x0821;
            s_cornerSprite->fillRect(posX + x * scale, posY, scale, scale, col);
        }
    }
}
