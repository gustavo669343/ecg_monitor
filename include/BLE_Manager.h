#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class BLEManager {
public:
    BLEManager();
    
    // Khởi tạo BLE với tên thiết bị tùy chọn
    void begin(const char* deviceName = "ESP32_ECG_AI_Tester");
    
    // Rút 1 mẫu dữ liệu ra khỏi bộ đệm (thread-safe)
    bool dequeue(float &val);
    
    // Gửi dữ liệu liên tục (Notify) về máy tính
    void sendData(float raw, float filtered, bool isPeak, float bpm = 0,
                  int ai_class = 0, const char* class_name = "N",
                  const float classPercents[5] = nullptr,
                  float spo2 = 0.0f, float ppg_bpm = 0.0f, uint32_t ir = 0);
    
    // Gửi debug message
    void sendDebugMessage(const char* message);
    
    // Kiểm tra trạng thái kết nối
    bool isConnected();

    // Các hàm này dùng nội bộ cho Callbacks (không gọi trong main)
    void enqueue(float val); // thread-safe
    void setConnected(bool state);

private:
    BLECharacteristic *pCharacteristic;
    bool deviceConnected;

    // --- BỘ ĐỆM FIFO THREAD-SAFE ---
    // Dùng FreeRTOS spinlock (portMUX) thay vì chỉ volatile int
    // để đảm bảo enqueue (BLE callback/Core 0) và dequeue (loop/Core 1)
    // không xảy ra race condition khi chạy đồng thời
    static const int BLE_BUFFER_SIZE = 4096;
    float bleQueue[BLE_BUFFER_SIZE];
    int qHead;
    int qTail;
    portMUX_TYPE queueMux; // Spinlock bảo vệ queue

    // UUIDs
    const char* SERVICE_UUID        = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
    const char* CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
};

#endif