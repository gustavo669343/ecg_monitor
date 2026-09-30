#ifndef ECG_AI_PROCESSOR_H
#define ECG_AI_PROCESSOR_H

#include <Arduino.h>
#include "model_3_branch_f32_leaky_final.h"
#include "BLE_Manager.h"

// Liên kết dữ liệu mô hình nơ-ron 3 nhánh CNN LeakyReLU
#ifndef model_data
#define model_data g_model_3_branch_f32_leaky
#endif
// --- THƯ VIỆN TENSORFLOW LITE MICRO ---
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

// --- CẤU HÌNH BẬT/TẮT CÁC TÍNH NĂNG HẬU XỬ LAI (SWTICHES) ---
#define ENABLE_RHYTHM_FILTER      0 // Công tắc 4: Bộ lọc nhịp điệu LBBB (Tắt để AI tự học)
#define ENABLE_TIMING_S_UPGRADE   0 // Công tắc 5: Quy tắc nâng cấp nhịp S (Tắt để AI tự học)
#define ENABLE_HYBRID_OVERRIDE    0 // Công tắc 6: Bộ đè tạo nhịp (Gai phần cứng) - Đặt 0 khi test offline các ca thường
#define ENABLE_FUSION_BYPASS      0 // Công tắc 7: Bypass SMA cho nhịp Fusion F
#define CONFIDENCE_THRESHOLD 65.0f

class ECGAIProcessor {
public:
    ECGAIProcessor(); // Constructor
    
    void begin();                 // Khởi tạo TFLite và biến
    void setBLEManager(BLEManager* ble); // Thiết lập BLE manager để gửi debug
    void processSample(float raw); // Hàm xử lý 1 mẫu dữ liệu mới
    
    // --- FIX RACE CONDITION: volatile đảm bảo Core 0 luôn đọc giá trị mới nhất từ Core 1 ---
    volatile bool readyForInference; 
    void runInference();
    
    // Các hàm để lấy kết quả ra ngoài
    float getBPM();
    int getProtectedClass();
    float getConfidence();
    float getFilteredSignal();
    bool isPeakDetected();
    void getClassPercentages(float out[5]);
    static const int NUM_CLASSES = 5;

    // --- BẢNG TÊN LỚP CHUẨN MIT-BIH ---
    // N=Normal, S=Supraventricular, V=Ventricular(PVC), F=Fusion, Q=Unknown/Paced
    static const char* CLASS_NAMES[NUM_CLASSES];

    // Trả về tên lớp theo index (ví dụ: 0 -> "N - Normal Beat")
    static const char* getClassName(int cls);

private:
    // --- CẤU HÌNH HỆ THỐNG ---
    static const int Fs = 125;
    uint32_t globalSampleCount;

    // --- BỘ LỌC NOTCH DUAL-FREQUENCY (50Hz / 60Hz) ---
    #define SELECT_NOTCH_60HZ 0  // Đặt 1 để lọc nhiễu 60Hz (dữ liệu Mỹ), đặt 0 để lọc nhiễu 50Hz (Việt Nam)

    float n_x[3], n_y[3];
    #if SELECT_NOTCH_60HZ
        // Hệ số lọc Notch 60Hz tại Fs = 125Hz
        const float b0 = 0.9391f, b1 = 1.8634f, b2 = 0.9391f, a1 = 1.8594f, a2 = 0.8782f;
    #else
        // Hệ số lọc Notch 50Hz tại Fs = 125Hz
        const float b0 = 0.9391f, b1 = 1.5195f, b2 = 0.9391f, a1 = 1.5195f, a2 = 0.8782f;
    #endif

    float estimated_baseline;
    float clean_signal;

    // --- THÊM CÁC BIẾN CHO BỘ LỌC ĐƯỜNG NỀN VÀ LÀM MƯỢT ---
    float high_freq_smoothed; // Biến mới để làm mượt răng cưa

    // --- BỘ LỌC BANDPASS ---
    float lp_out, hp_out;
    float prev_raw_n, prev_lp, prev_hp;

    // --- PAN-TOMPKINS ---
    float diff_buffer[3];
    static const int WINDOW_SIZE = 18;
    float window[WINDOW_SIZE];
    int windowIdx;
    float windowSum;
    float mwi_out;
    float threshold; 
    uint32_t lastPeakSampleIndex;
    uint32_t secondLastPeakSampleIndex;
    float lastRRRatio;
    float heartRate;
    float smoothedBPM;
    float prevSmoothedBPM;
    const float bpmAlpha = 0.1f;
    bool peakDetectedFlag;

    // --- AUTO-CALIBRATION NGƯỠNG QRS ---
    // Trong 250 mẫu đầu (2 giây), hệ thống tự học biên độ tín hiệu
    // thay vì dùng ngưỡng cứng 80.0 không phù hợp mọi tín hiệu
    static const int CALIBRATION_SAMPLES = 350;
    bool calibrationDone;
    float calibrationMaxMWI;

    // --- AI & BUFFER ---
    static const int MODEL_INPUT_SIZE = 375;
    static const int BUFFER_SIZE = 512;
    float ecgBuffer[BUFFER_SIZE]; 
    int bufferHead;
    
    struct PendingInference {
        int peak_buffer_idx;
        uint32_t peak_global_idx;
        uint32_t target_count;
    };
    static const int MAX_PENDING_INFERENCES = 8;
    PendingInference pending_queue[MAX_PENDING_INFERENCES];
    int queue_head;
    int queue_tail;
    int queue_size;

    void enqueue_inference(int buffer_idx, uint32_t global_idx, uint32_t target);
    bool dequeue_inference(PendingInference &out_item);

    int inference_peak_buffer_idx;
    uint32_t inference_peak_global_idx;
    static const int SAMPLES_TO_WAIT = 124;

    // --- TFLITE ---
    const tflite::Model* model;
    tflite::MicroInterpreter* interpreter;
    TfLiteTensor* input;
    TfLiteTensor* output;
    tflite::ErrorReporter* errorReporter;

    static const int kTensorArenaSize = 100 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];

    // --- LỚP BẢO VỆ KẾT QUẢ ---
    static const int PROB_WINDOW = 1;        // Khung cửa sổ nhớ 1 nhịp (Bỏ lọc SMA để AI nhận diện nhạy nhịp V)
    
    // Mảng 2 chiều lưu lịch sử xác suất: [Số_lớp][Số_nhịp_nhớ]
    float probHistory[NUM_CLASSES][PROB_WINDOW]; 
    int probHistoryIdx;
    
    int finalProtectedResult;
    float currentConfidence;
    float classPercentages[NUM_CLASSES];
    
    // --- BỘ PHÁT HIỆN GAI TẠO NHỊP (PACEMAKER SPIKE) ---
    float prevRawSample;
    uint32_t lastSpikeSampleIndex;
    bool hasPacingSpikeInWindow;
    
    BLEManager* bleManager;
};

#endif