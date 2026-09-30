#include "MAX30102_Manager.h"

MAX30102Manager::MAX30102Manager() {
    sensorInitialized = false;
    rawIR = 0;
    rawRed = 0;
    ppgBPM = 0.0f;
    spo2Value = 0.0f;
    fingerDetected = false;
    rateSpot = 0;
    lastBeat = 0;
    irAC = 0; irDC = 0;
    redAC = 0; redDC = 0;
    prevIR = 0; prevRed = 0;
    memset(rates, 0, sizeof(rates));
}

bool MAX30102Manager::begin(int sdaPin, int sclPin) {
    Wire.begin(sdaPin, sclPin, 400000);

    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.printf("❌ CẢNH BÁO: Không tìm thấy cảm biến MAX30102 trên đường I2C (SDA=%d, SCL=%d)!\n", sdaPin, sclPin);
        sensorInitialized = false;
        return false;
    }

    // Cấu hình cảm biến tối ưu:
    // ledBrightness = 60, sampleAverage = 4, ledMode = 2 (Red + IR), sampleRate = 100, pulseWidth = 411, adcRange = 4096
    particleSensor.setup(60, 4, 2, 100, 411, 4096);
    particleSensor.setPulseAmplitudeRed(0x24); // Cường độ LED Đỏ
    particleSensor.setPulseAmplitudeIR(0x24);  // Cường độ LED Hồng ngoại
    particleSensor.setPulseAmplitudeGreen(0);  // Tắt LED Xanh

    sensorInitialized = true;
    Serial.println("✅ HỆ THỐNG: Cảm biến MAX30102 (SpO2 & PPG) đã khởi tạo thành công!");
    return true;
}

void MAX30102Manager::update() {
    if (!sensorInitialized) return;

    // Đọc mẫu mới nhất từ cảm biến
    rawIR = particleSensor.getIR();
    rawRed = particleSensor.getRed();

    // 1. Kiểm tra xem người dùng có áp ngón tay vào cảm biến không
    if (rawIR < FINGER_THRESHOLD) {
        fingerDetected = false;
        ppgBPM = 0.0f;
        spo2Value = 0.0f;
        rateSpot = 0;
        lastBeat = 0;
        return;
    }

    fingerDetected = true;

    // 2. Phát hiện nhịp tim qua sóng PPG hồng ngoại (IR)
    if (checkForBeat(rawIR)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        if (delta > 300 && delta < 2000) { // Giới hạn nhịp sinh lý (30 - 200 BPM)
            float beatsPerMinute = 60000.0f / (float)delta;
            rates[rateSpot++] = (byte)beatsPerMinute;
            rateSpot %= RATE_SIZE;

            // Tính trung bình các lần đập
            int beatAvg = 0;
            byte count = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
                if (rates[x] > 0) {
                    beatAvg += rates[x];
                    count++;
                }
            }
            if (count > 0) {
                ppgBPM = (float)beatAvg / count;
            }
        }
    }

    // 3. Tính toán nồng độ SpO2 bằng tỷ số R (AC/DC Ratio)
    // Tách thành phần một chiều (DC) qua bộ lọc thông thấp IIR
    if (irDC == 0) {
        irDC = rawIR;
        redDC = rawRed;
    } else {
        irDC = 0.95f * irDC + 0.05f * (float)rawIR;
        redDC = 0.95f * redDC + 0.05f * (float)rawRed;
    }

    // Thành phần xoay chiều (AC)
    float current_irAC = fabsf((float)rawIR - irDC);
    float current_redAC = fabsf((float)rawRed - redDC);

    irAC = 0.9f * irAC + 0.1f * current_irAC;
    redAC = 0.9f * redAC + 0.1f * current_redAC;

    if (irDC > 1000 && redDC > 1000 && irAC > 10) {
        float ratio = (redAC / redDC) / (irAC / irDC);
        // Công thức thực nghiệm chuẩn cho MAX30102
        float calculatedSpO2 = 110.0f - 25.0f * ratio;

        if (calculatedSpO2 > 100.0f) calculatedSpO2 = 99.5f;
        if (calculatedSpO2 < 70.0f) calculatedSpO2 = 70.0f;

        // Làm mịn giá trị hiển thị
        if (spo2Value == 0) {
            spo2Value = calculatedSpO2;
        } else {
            spo2Value = 0.92f * spo2Value + 0.08f * calculatedSpO2;
        }
    }
}

uint32_t MAX30102Manager::getRawIR() const { return rawIR; }
uint32_t MAX30102Manager::getRawRed() const { return rawRed; }
float MAX30102Manager::getBPM() const { return ppgBPM; }
float MAX30102Manager::getSpO2() const { return spo2Value; }
bool MAX30102Manager::isFingerDetected() const { return fingerDetected; }
