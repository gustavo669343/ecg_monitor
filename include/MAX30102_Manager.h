#ifndef MAX30102_MANAGER_H
#define MAX30102_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

class MAX30102Manager {
public:
    MAX30102Manager();

    // Khởi tạo I2C và cảm biến MAX30102
    bool begin(int sdaPin = 8, int sclPin = 9);

    // Cập nhật lấy mẫu dữ liệu từ cảm biến
    void update();

    // Lấy các chỉ số
    uint32_t getRawIR() const;
    uint32_t getRawRed() const;
    float getBPM() const;
    float getSpO2() const;
    bool isFingerDetected() const;

private:
    MAX30105 particleSensor;
    bool sensorInitialized;

    uint32_t rawIR;
    uint32_t rawRed;
    float ppgBPM;
    float spo2Value;
    bool fingerDetected;

    // Bộ đệm tính toán nhịp tim PPG
    static const byte RATE_SIZE = 4;
    byte rates[RATE_SIZE];
    byte rateSpot;
    long lastBeat;

    // Bộ lọc AC/DC cho SpO2
    float irAC, irDC;
    float redAC, redDC;
    float prevIR, prevRed;

    // Ngưỡng phát hiện ngón tay đặt lên cảm biến
    static const uint32_t FINGER_THRESHOLD = 30000;
};

#endif // MAX30102_MANAGER_H
