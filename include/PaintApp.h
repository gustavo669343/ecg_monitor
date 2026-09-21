#pragma once
#include <Arduino.h>
#include "DisplayManager.h"

struct UI_Button {
    int16_t x, y, w, h;
    uint16_t color;
    const char* label;

    bool contains(uint16_t px, uint16_t py, int16_t pad = 4) const {
        return (px >= (x - pad) && px <= (x + w + pad) && py >= (y - pad) && py <= (y + h + pad));
    }

};

class CornerGif;

/**
 * @brief Application layer that provides an interactive touch painting canvas,
 * color palette selector, coordinate telemetry, and action buttons.
 */
class PaintApp {
public:
    explicit PaintApp(DisplayManager& displayManager);

    /**
     * @brief Renders initial UI and prepares drawing canvas.
     */
    void init();

    /**
     * @brief Gán con trỏ tới CornerGif để hỗ trợ chạm đổi ảnh và vẽ lại khi xoá màn hình.
     */
    void setCornerGif(CornerGif* gif) { cornerGif = gif; }

    /**
     * @brief Redraws the full interface.
     */
    void drawUI();

    /**
     * @brief Clears the drawing canvas area.
     */
    void clearCanvas();

    /**
     * @brief Polls touch input and handles button presses / brush strokes.
     */
    void update();

private:
    DisplayManager& display;
    CornerGif* cornerGif = nullptr;
    uint16_t currentColor;
    uint8_t brushRadius;


    static constexpr uint8_t NUM_COLOR_BTNS = 5;
    UI_Button colorBtns[NUM_COLOR_BTNS];
    UI_Button btnClear;
    UI_Button btnCalib;

    void updateCoordinates(uint16_t x, uint16_t y);
    void updateHeaderName();
    void updateColorIndicator();

    int16_t lastX = -1;
    int16_t lastY = -1;
    char lastGifName[32] = "";
};

