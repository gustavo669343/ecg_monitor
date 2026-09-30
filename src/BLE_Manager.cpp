#include "BLE_Manager.h"

// --- CÁC LỚP CALLBACKS CỦA BLE ---
class MyServerCallbacks: public BLEServerCallbacks {
    BLEManager* manager;
public:
    MyServerCallbacks(BLEManager* mgr) : manager(mgr) {}
    void onConnect(BLEServer* pServer) { 
        manager->setConnected(true); 
        Serial.println("[BLE] Đã kết nối!");
    }
    void onDisconnect(BLEServer* pServer) {
        manager->setConnected(false);
        pServer->getAdvertising()->start();
        Serial.println("[BLE] Mất kết nối. Đang chờ kết nối lại...");
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    BLEManager* manager;
public:
    MyCharacteristicCallbacks(BLEManager* mgr) : manager(mgr) {}
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            // Tách các giá trị số từ file CSV gửi qua
            char* token = strtok(const_cast<char*>(value.c_str()), ",\n\r");
            while (token != NULL) {
                float val = atof(token);
                manager->enqueue(val); // Thread-safe enqueue
                token = strtok(NULL, ",\n\r");
            }
        }
    }
};

// --- IMPLEMENTATION CỦA BLEManager ---

BLEManager::BLEManager() {
    deviceConnected = false;
    qHead = 0;
    qTail = 0;
    pCharacteristic = nullptr;
    // Khởi tạo spinlock
    queueMux = portMUX_INITIALIZER_UNLOCKED;
}

void BLEManager::begin(const char* deviceName) {
    BLEDevice::init(deviceName);
    BLEServer *pServer = BLEDevice::createServer();
    
    // Truyền con trỏ (this) để Callback có thể gọi hàm của lớp này
    pServer->setCallbacks(new MyServerCallbacks(this));

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(this));
    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("[BLE] Khởi tạo thành công. Đang phát sóng...");
}

// --- THREAD-SAFE ENQUEUE (gọi từ BLE callback, có thể chạy trên Core 0) ---
void BLEManager::enqueue(float val) {
    portENTER_CRITICAL(&queueMux);
    int next = (qHead + 1) % BLE_BUFFER_SIZE;
    if (next != qTail) { 
        bleQueue[qHead] = val;
        qHead = next;
    }
    portEXIT_CRITICAL(&queueMux);
}

// --- THREAD-SAFE DEQUEUE (gọi từ loop(), chạy trên Core 1) ---
bool BLEManager::dequeue(float &val) {
    portENTER_CRITICAL(&queueMux);
    if (qHead == qTail) {
        portEXIT_CRITICAL(&queueMux);
        return false; 
    }
    val = bleQueue[qTail];
    qTail = (qTail + 1) % BLE_BUFFER_SIZE;
    portEXIT_CRITICAL(&queueMux);
    return true;
}

// --- GỬI DỮ LIỆU BLE (bao gồm tên lớp AI & dữ liệu MAX30102) ---
void BLEManager::sendData(float raw, float filtered, bool isPeak, float bpm,
                          int ai_class, const char* class_name,
                          const float classPercents[5],
                          float spo2, float ppg_bpm, uint32_t ir) {
    if (deviceConnected && pCharacteristic != nullptr) {
        char resultStr[200];
        
        if (classPercents != nullptr) {
            // Gửi kèm tên lớp AI (N/S/V/F/Q) và phần trăm từng lớp
            if (isPeak) {
                snprintf(resultStr, sizeof(resultStr),
                         "R:%.1f|F:%.1f|P:1|B:%.1f|C:%d|CN:%s|Q:%.0f,%.0f,%.0f,%.0f,%.0f|SPO2:%.1f|PBPM:%.1f|IR:%u",
                         raw, filtered, bpm, ai_class, class_name,
                         classPercents[0], classPercents[1], classPercents[2],
                         classPercents[3], classPercents[4],
                         spo2, ppg_bpm, ir);
            } else {
                snprintf(resultStr, sizeof(resultStr),
                         "R:%.1f|F:%.1f|P:0|Q:%.0f,%.0f,%.0f,%.0f,%.0f|SPO2:%.1f|PBPM:%.1f|IR:%u",
                         raw, filtered,
                         classPercents[0], classPercents[1], classPercents[2],
                         classPercents[3], classPercents[4],
                         spo2, ppg_bpm, ir);
            }
        } else {
            // Nếu không có dữ liệu phần trăm lớp
            if (isPeak) {
                snprintf(resultStr, sizeof(resultStr),
                         "R:%.1f|F:%.1f|P:1|B:%.1f|C:%d|CN:%s|SPO2:%.1f|PBPM:%.1f|IR:%u",
                         raw, filtered, bpm, ai_class, class_name,
                         spo2, ppg_bpm, ir);
            } else {
                snprintf(resultStr, sizeof(resultStr),
                         "R:%.1f|F:%.1f|P:0|SPO2:%.1f|PBPM:%.1f|IR:%u",
                         raw, filtered, spo2, ppg_bpm, ir);
            }
        }
        
        pCharacteristic->setValue(resultStr);
        pCharacteristic->notify();
    }
}

bool BLEManager::isConnected() {
    return deviceConnected;
}

void BLEManager::setConnected(bool state) {
    deviceConnected = state;
}

void BLEManager::sendDebugMessage(const char* message) {
    if (deviceConnected && pCharacteristic != nullptr) {
        char debugStr[256];
        snprintf(debugStr, sizeof(debugStr), "DBG:%s", message);
        pCharacteristic->setValue(debugStr);
        pCharacteristic->notify();
    }
}
