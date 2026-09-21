#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "DisplayManager.h"

/**
 * @brief Floating Sprite component for static RGBA/RGB bitmap images.
 * Moves smoothly across the screen with bounce physics, zero flicker, and transparency.
 */
class FloatingSprite {
public:
    explicit FloatingSprite(DisplayManager& displayManager);
    ~FloatingSprite();

    /**
     * @brief Loads a 32-bit RGBA8888 bitmap array into a 16-bit TFT_eSprite.
     * @param pRgbaData Pointer to array of uint32_t (0xRRGGBBAA format)
     * @param w Width in pixels
     * @param h Height in pixels
     * @param startX Initial X coordinate
     * @param startY Initial Y coordinate
     * @param speedX Horizontal velocity
     * @param speedY Vertical velocity
     * @return true if loaded successfully
     */
    bool loadRGBA(const uint32_t* pRgbaData, uint16_t w, uint16_t h, 
                  float startX = 200, float startY = 150, 
                  float speedX = -1.5, float speedY = 1.3);

    /**
     * @brief Sets the bounding box for the sprite movement.
     */
    void setBounds(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY);

    /**
     * @brief Updates movement tick. Call non-blockingly in loop().
     */
    void update();

    /**
     * @brief Stops and frees sprite buffer memory.
     */
    void stop();

    uint16_t getWidth() const { return spriteWidth; }
    uint16_t getHeight() const { return spriteHeight; }

private:
    DisplayManager& display;
    TFT_eSprite sprite;

    bool isLoaded;
    uint16_t spriteWidth;
    uint16_t spriteHeight;

    // Position & velocity
    float x, y;
    int16_t prevX, prevY;
    float vx, vy;

    // Boundary constraints
    int16_t boundMinX, boundMinY, boundMaxX, boundMaxY;

    uint32_t lastMoveTime;

    void eraseOldPosition();
};
