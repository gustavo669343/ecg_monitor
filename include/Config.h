#pragma once
#include <Arduino.h>

// ==========================================
// Hardware & Pinout Configuration
// ==========================================
constexpr uint8_t TFT_LED_PIN = 14;   // GPIO connected to LED backlight
// TUYỆT ĐỐI KHÔNG DÙNG GPIO 4 (gây lỗi boot trên ESP32-S3)
// Đổi T_IRQ sang GPIO 47 (hoặc đặt = -1 nếu không muốn nối chân T_IRQ)
constexpr int8_t  TOUCH_IRQ_PIN = 47;  // GPIO 47 kết nối T_IRQ (Chân 14 màn hình)

// ==========================================
// Biomedical Sensors (AD8232 ECG & MAX30102 Pulse Oximeter)
// ==========================================
// AD8232 ECG Sensor
constexpr uint8_t AD8232_OUTPUT_PIN   = 1;  // GPIO 1 (ADC1_CH0 - đọc tín hiệu điện tim ECG)
constexpr int8_t  AD8232_LO_PLUS_PIN  = 2;  // GPIO 2 (Tùy chọn: Phát hiện tuột điện cực LO+)
constexpr int8_t  AD8232_LO_MINUS_PIN = 3;  // GPIO 3 (Tùy chọn: Phát hiện tuột điện cực LO-)

// MAX30102 Heart Rate & SpO2 Sensor (I2C)
constexpr uint8_t MAX30102_SDA_PIN    = 5;  // GPIO 5 (I2C SDA)
constexpr uint8_t MAX30102_SCL_PIN    = 6;  // GPIO 6 (I2C SCL)
constexpr int8_t  MAX30102_INT_PIN    = 7;  // GPIO 7 (Tùy chọn: Chân ngắt INT)

// ==========================================
// MicroSD Card Module (Dedicated SPI Bus - Option 2)
// Tránh 100% xung đột bus MISO với chip cảm ứng XPT2046
// ==========================================
constexpr uint8_t SD_CS_PIN   = 8;   // GPIO 8  (SD Chip Select)
constexpr uint8_t SD_SCK_PIN  = 17;  // GPIO 17 (SD SPI Clock)
constexpr uint8_t SD_MOSI_PIN = 18;  // GPIO 18 (SD SPI MOSI / DI)
constexpr uint8_t SD_MISO_PIN = 21;  // GPIO 21 (SD SPI MISO / DO)

// ==========================================
// Display Dimensions & Rotation
// ==========================================
constexpr uint16_t SCREEN_WIDTH = 480;
constexpr uint16_t SCREEN_HEIGHT = 320;
constexpr uint8_t SCREEN_ROTATION = 1; // 1 = Landscape (480x320)

// ==========================================
// Touch Calibration & Sensitivity
// ==========================================
// Set to true if you want to run calibration on every boot
constexpr bool FORCE_CALIBRATION_ON_BOOT = false;

// Ngưỡng áp lực nhạy (hạ xuống 80 để ngón tay chạm nhẹ cũng nhận)
constexpr uint16_t TOUCH_PRESSURE_THRESHOLD = 80;

// Dữ liệu cân chỉnh thực tế đo được từ màn hình của bạn
const uint16_t DEFAULT_CAL_DATA[5] = { 234, 3561, 142, 3472, 7 };

