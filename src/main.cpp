#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"
#include "AppUI.h"
#include "CornerGif.h"
#include "GifRegistry.h"
#include "ECG_AI_Processor.h"
#include "BLE_Manager.h"
#include "MAX30102_Manager.h"

// --- KHỞI TẠO CÁC ĐỐI TƯỢNG HỆ THỐNG ---
DisplayManager display(TFT_LED_PIN);
AppUI appUI(display);
CornerGif cornerGif(display);

ECGAIProcessor ecgProcessor;
BLEManager bleManager;
MAX30102Manager max30102;

// Handles cho các Task FreeRTOS
TaskHandle_t AI_TaskHandle;
TaskHandle_t MAX30102_TaskHandle;
TaskHandle_t ECG_TaskHandle;

// Task 1: Mạng nơ-ron AI suy luận phân loại nhịp (chạy trên Core 0)
void AITask(void *parameter) {
    for (;;) { 
        if (appUI.isAiEnabled()) {
            if (ecgProcessor.readyForInference) {
                ecgProcessor.runInference();
                ecgProcessor.readyForInference = false; 
            }
        } else {
            ecgProcessor.readyForInference = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}

// Task 2: Cảm biến quang SpO2 & PPG MAX30102 (chạy trên Core 0)
void MAX30102Task(void *parameter) {
    for (;;) {
        max30102.update();
        vTaskDelay(10 / portTICK_PERIOD_MS); // Tần số lấy mẫu ~100Hz
    }
}

// Task 3: LẤY MẪU ECG 125Hz ĐỘC LẬP VỚI ĐỘ ƯU TIÊN CAO NHẤT (Chạy trên Core 1)
// Sử dụng vTaskDelayUntil để đảm bảo chu kỳ 8ms ngặt nghèo, loại bỏ 100% hiện tượng drop mẫu
void ECGSamplingTask(void *parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(8); // Chu kỳ chuẩn 8ms = 125Hz

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        float inputSample = 0;
        
        // Kiểm tra tuột điện cực (Lead-Off detection)
        if ((digitalRead(AD8232_LO_PLUS_PIN) == 1) || (digitalRead(AD8232_LO_MINUS_PIN) == 1)) {
            inputSample = 0; 
        } else {
            inputSample = analogRead(AD8232_OUTPUT_PIN); // GPIO 1 (ADC1_CH0)
        }
        
        // Đưa mẫu qua khối xử lý số tín hiệu & Pan-Tompkins
        ecgProcessor.processSample(inputSample);

        // Lấy kết quả điện tim & AI
        float filtered = ecgProcessor.getFilteredSignal();
        float bpm      = ecgProcessor.getBPM();
        int   ai_class = ecgProcessor.getProtectedClass();
        bool  isPeak   = ecgProcessor.isPeakDetected();
        float classPercentages[ECGAIProcessor::NUM_CLASSES] = {0};
        ecgProcessor.getClassPercentages(classPercentages);
        const char* class_name = ECGAIProcessor::getClassName(ai_class);

        // Lấy dữ liệu từ MAX30102
        float spo2_val  = max30102.getSpO2();
        float ppg_bpm   = max30102.getBPM();
        uint32_t raw_ir = max30102.getRawIR();

        // 1. Cập nhật mẫu sóng ECG thực (cả kênh thô và kênh lọc) vào bộ đệm hiển thị trên màn hình TFT
        appUI.pushRealECGSamples(inputSample, filtered);

        // 2. Cập nhật các chỉ số sinh hiệu vào Dashboard UI
        appUI.setBiometrics(bpm, spo2_val, ppg_bpm, ai_class, class_name);

        // 3. Gửi đồng thời gói tin qua BLE Notify về PC / App di động
        bleManager.sendData(inputSample, filtered, isPeak, bpm,
                            ai_class, class_name, classPercentages,
                            spo2_val, ppg_bpm, raw_ir);

        // Log khi phát hiện đỉnh R
        if (isPeak) {
            Serial.printf("[%lu ms] ECG BPM: %.1f | SpO2: %.1f%% | PPG BPM: %.1f | Class: %d (%s)\n",
                          millis(), bpm, spo2_val, ppg_bpm, ai_class, class_name);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println("\n=======================================================");
    Serial.println("  ESP32-S3: HE THONG GIAM SAT DIEN TIM ECG & SPO2 (VER 4) ");
    Serial.println("=======================================================");

    // 1. Cấu hình chân phần cứng cảm biến AD8232 (GPIO 1, 2, 3)
    pinMode(AD8232_LO_PLUS_PIN, INPUT);
    pinMode(AD8232_LO_MINUS_PIN, INPUT);

    // 2. Khởi tạo Màn hình TFT LCD ILI9488 & Cảm ứng XPT2046
    Serial.println("[1/5] Khoi tao man hinh TFT LCD ILI9488...");
    display.init(SCREEN_ROTATION);
    display.setCalibration(DEFAULT_CAL_DATA);
    Serial.println("[1/5] Man hinh & Cam ung san sang!");

    // 3. Khởi tạo Giao diện Dashboard đa tab & Linh vật Ami
    Serial.println("[2/5] Khoi tao Dashboard UI & Linh vat Mascot...");
    appUI.init();
    appUI.setCornerGif(&cornerGif);
    cornerGif.setPosition(278, 40);
    cornerGif.playByName("normal");

    // 4. Khởi tạo Mạng AI chẩn đoán ECG
    Serial.println("[3/5] Khoi tao mang no-ron AI TensorFlow Lite...");
    ecgProcessor.begin();
    ecgProcessor.setBLEManager(&bleManager);

    // 5. Khởi tạo Cảm biến SpO2 MAX30102 qua I2C (SDA=GPIO 5, SCL=GPIO 6)
    Serial.println("[4/5] Khoi tao cam bien MAX30102 (I2C SDA=5, SCL=6)...");
    max30102.begin(MAX30102_SDA_PIN, MAX30102_SCL_PIN);

    // 6. Khởi tạo Bluetooth BLE
    Serial.println("[5/5] Khoi tao Bluetooth BLE...");
    bleManager.begin("ESP32_ECG_AI_Tester");

    // 7. Tạo các FreeRTOS Tasks:
    // Core 0: AI Inference & MAX30102
    xTaskCreatePinnedToCore(AITask, "AI_Inference", 15000, NULL, 2, &AI_TaskHandle, 0);
    xTaskCreatePinnedToCore(MAX30102Task, "MAX30102_Task", 4096, NULL, 1, &MAX30102_TaskHandle, 0);

    // Core 1: Lấy mẫu ECG 125Hz với độ ưu tiên cao nhất (Priority 10)
    // Ngăn chặn hoàn toàn việc vẽ màn hình làm chậm hoặc drop mẫu ECG!
    xTaskCreatePinnedToCore(ECGSamplingTask, "ECG_Sample", 8192, NULL, 10, &ECG_TaskHandle, 1);

    Serial.println(">> TOAN BO HE THONG DA KHOI DONG THANH CONG! <<\n");
}

void loop() {
    // Vòng lặp loop chạy ở độ ưu tiên thấp (Priority 1 trên Core 1)
    // Khi đến chu kỳ 8ms, ECGSamplingTask (Priority 10) sẽ tự động chiếm quyền CPU 50us để lấy mẫu
    // và nhường lại để loop tiếp tục vẽ màn hình mượt mà không bao giờ mất mẫu ECG!

    // Cập nhật hoạt hình linh vật Ami (khi ở Trang Chủ)
    if (appUI.getCurrentView() == VIEW_MAIN) {
        cornerGif.update();
    }

    // Cập nhật màn hình LCD, cảm ứng và các tab
    appUI.update();

    vTaskDelay(2 / portTICK_PERIOD_MS);
}
