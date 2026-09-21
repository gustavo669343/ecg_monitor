#pragma once
#include <Arduino.h>
#include <AnimatedGIF.h>
#include <TFT_eSPI.h>
#include "DisplayManager.h"

/**
 * @brief Floating GIF Sprite component.
 * Plays a small animated GIF (e.g. twinkling star, pet, particle) and moves it
 * smoothly across the screen with bounce physics, zero flicker, and transparency.
 */
class FloatingGif {
public:
    explicit FloatingGif(DisplayManager& displayManager);
    ~FloatingGif();

    /**
     * @brief Loads and initializes a GIF from Flash memory (PROGMEM).
     * @param pGifData Pointer to byte array of the GIF
     * @param size Size in bytes
     * @param startX Initial X coordinate
     * @param startY Initial Y coordinate
     * @param speedX Horizontal velocity (pixels per move tick)
     * @param speedY Vertical velocity (pixels per move tick)
     * @return true if loaded successfully
     */
    bool load(const uint8_t* pGifData, size_t size, float startX = 100, float startY = 100, float speedX = 1.5, float speedY = 1.2);

    /**
     * @brief Sets the bounding box for the sprite movement (e.g., to keep it inside the canvas).
     */
    void setBounds(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY);

    /**
     * @brief Updates movement and frame animation. Must be called in loop() non-blockingly.
     */
    void update();

    /**
     * @brief Stops playback and frees sprite memory.
     */
    void stop();

    uint16_t getWidth() const { return gifWidth; }
    uint16_t getHeight() const { return gifHeight; }

    // Static callback for AnimatedGIF line decode
    static void GIFDrawCallback(GIFDRAW *pDraw);

private:
    DisplayManager& display;
    AnimatedGIF gif;
    TFT_eSprite sprite;

    bool isLoaded;
    uint16_t gifWidth;
    uint16_t gifHeight;

    // Position & velocity
    float x, y;
    int16_t prevX, prevY;
    float vx, vy;

    // Movement boundary
    int16_t boundMinX, boundMinY, boundMaxX, boundMaxY;

    // Timers
    uint32_t lastMoveTime;
    uint32_t lastFrameTime;
    int frameDelayMs;

    void decodeNextFrame();
    void eraseOldPosition();
};
