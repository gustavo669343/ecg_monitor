#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Config.h"

/**
 * @brief Manages the TFT display and XPT2046 touch controller.
 * Handles backlight power, driver initialization, and touch calibration.
 */
class DisplayManager {
public:
    explicit DisplayManager(uint8_t backlightPin = TFT_LED_PIN);

    /**
     * @brief Initializes backlight GPIO, TFT driver, and screen rotation.
     */
    void init(uint8_t rotation = SCREEN_ROTATION);

    /**
     * @brief Turns display backlight ON or OFF.
     */
    void setBacklight(bool enable);

    /**
     * @brief Adjusts backlight brightness using PWM (0..255).
     */
    void setBrightness(uint8_t duty);

    /**
     * @brief Applies hardcoded or saved calibration array.
     */
    void setCalibration(const uint16_t data[5]);

    /**
     * @brief Runs an interactive 4-corner calibration and prints values to Serial.
     */
    void runCalibration();

    /**
     * @brief Checks if screen is touched and returns calibrated coordinates.
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @return true if touched, false otherwise
     */
    bool getTouch(uint16_t* x, uint16_t* y, uint16_t threshold = TOUCH_PRESSURE_THRESHOLD);

    /**
     * @brief Checks raw touch pressure (Z). Useful to diagnose wiring.
     */
    uint16_t getTouchRawZ() { return tft.getTouchRawZ(); }

    /**
     * @brief Reads uncalibrated raw X, Y coordinates from XPT2046.
     */
    bool getTouchRaw(uint16_t* x, uint16_t* y) { return tft.getTouchRaw(x, y); }

    /**
     * @brief Returns a reference to the underlying TFT_eSPI instance for drawing.
     */
    TFT_eSPI& getTft() { return tft; }

private:
    TFT_eSPI tft;
    uint8_t ledPin;
    uint16_t calData[5];
};
