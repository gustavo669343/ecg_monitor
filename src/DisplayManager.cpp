#include "DisplayManager.h"

DisplayManager::DisplayManager(uint8_t backlightPin)
    : ledPin(backlightPin) {
    memset(calData, 0, sizeof(calData));
}

void DisplayManager::init(uint8_t rotation) {
    // 1. Enable Backlight
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    // 2. Setup T_IRQ pin if available
    if (TOUCH_IRQ_PIN >= 0) {
        pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);
    }

    // 3. Initialize TFT controller
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);
}

void DisplayManager::setBacklight(bool enable) {
    digitalWrite(ledPin, enable ? HIGH : LOW);
}

void DisplayManager::setBrightness(uint8_t duty) {
    analogWrite(ledPin, duty);
}

void DisplayManager::setCalibration(const uint16_t data[5]) {
    memcpy(calData, data, sizeof(calData));
    tft.setTouch(calData);
    Serial.println("[DisplayManager] Applied touch calibration data.");
}

void DisplayManager::runCalibration() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("TOUCH CALIBRATION", tft.width() / 2, 30);
    tft.drawString("Touch the red crosshairs at the 4 corners...", tft.width() / 2, 60);

    // Run TFT_eSPI interactive calibration
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    // Output formatted snippet to Serial
    Serial.println("\n=======================================================");
    Serial.println("  CALIBRATION COMPLETE! COPY THIS LINE INTO Config.h:");
    Serial.printf("  const uint16_t DEFAULT_CAL_DATA[5] = { %u, %u, %u, %u, %u };\n",
                  calData[0], calData[1], calData[2], calData[3], calData[4]);
    Serial.println("=======================================================\n");

    tft.setTouch(calData);
    delay(500);
}

bool DisplayManager::getTouch(uint16_t* x, uint16_t* y, uint16_t threshold) {
    // 1. Kiểm tra chân ngắt vật lý T_IRQ (nếu HIGH = không có lực ấn -> thoát ngay trong 1us)
    if (TOUCH_IRQ_PIN >= 0 && digitalRead(TOUCH_IRQ_PIN) == HIGH) {
        return false;
    }

    // 2. Đọc toạ độ và kiểm tra hợp lệ
    return tft.getTouch(x, y, threshold);
}


