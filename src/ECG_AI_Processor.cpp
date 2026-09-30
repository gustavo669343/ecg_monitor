#include "ECG_AI_Processor.h"

// --- ĐỊNH NGHĨA BẢNG TÊN LỚP MIT-BIH ---
// Chuẩn phân loại nhịp tim quốc tế (AAMI EC57)
const char* ECGAIProcessor::CLASS_NAMES[ECGAIProcessor::NUM_CLASSES] = {
    "N - Normal Beat",          // Class 0: Nhịp bình thường
    "S - Supraventricular",     // Class 1: Nhịp nhanh trên thất
    "V - Ventricular (PVC)",    // Class 2: Ngoại tâm thu thất
    "F - Fusion Beat",          // Class 3: Nhịp hỗn hợp
    "Q - Unknown / Paced"       // Class 4: Không xác định / Nhịp máy tạo nhịp
};

const char* ECGAIProcessor::getClassName(int cls) {
    if (cls >= 0 && cls < NUM_CLASSES) return CLASS_NAMES[cls];
    return "? - Unknown";
}

ECGAIProcessor::ECGAIProcessor() {
    // Khởi tạo các giá trị mặc định bằng 0
    globalSampleCount = 0;
    memset(n_x, 0, sizeof(n_x)); memset(n_y, 0, sizeof(n_y));
    lp_out = 0; hp_out = 0; prev_raw_n = 0; prev_lp = 0; prev_hp = 0;

    estimated_baseline = 0;
    high_freq_smoothed = 0;
    clean_signal = 0;

    memset(diff_buffer, 0, sizeof(diff_buffer));
    memset(window, 0, sizeof(window));
    windowIdx = 0; windowSum = 0; mwi_out = 0;

    // --- AUTO-CALIBRATION: khởi tạo trạng thái học ngưỡng ---
    // Ngưỡng ban đầu đặt 0 trong giai đoạn calibration
    threshold = 0.0;
    calibrationDone = false;
    calibrationMaxMWI = 0.0;

    lastPeakSampleIndex = 0;
    secondLastPeakSampleIndex = 0;
    heartRate = 0; smoothedBPM = 0; prevSmoothedBPM = 0.0f; peakDetectedFlag = false;
    
    // Khởi tạo hàng đợi đa luồng suy luận
    queue_head = 0;
    queue_tail = 0;
    queue_size = 0;
    inference_peak_buffer_idx = 0;
    inference_peak_global_idx = 0;
    
    // Khởi tạo các biến phát hiện gai tạo nhịp
    prevRawSample = 0.0f;
    lastSpikeSampleIndex = 0;
    hasPacingSpikeInWindow = false;

    bleManager = nullptr;
}

void ECGAIProcessor::begin() {
    model = tflite::GetModel(model_data);

    // Kiểm tra phiên bản của mô hình
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("LỖI AI: Phiên bản mô hình TFLite không tương thích với TFLM!");
        return;
    }

    static tflite::MicroMutableOpResolver<20> resolver;
    
    resolver.AddConv2D(); 
    resolver.AddMaxPool2D(); 
    resolver.AddFullyConnected();
    resolver.AddReshape(); 
    resolver.AddRelu(); 
    resolver.AddLeakyRelu(); // Bổ sung toán tử LeakyReLU cho mô hình 3 nhánh CNN
    resolver.AddSoftmax();
    resolver.AddAdd(); 
    resolver.AddQuantize(); 
    resolver.AddDequantize();
    resolver.AddExpandDims();
    resolver.AddMean();
    resolver.AddShape();
    resolver.AddStridedSlice();
    resolver.AddPack();
    resolver.AddConcatenation(); // [MODEL 4] Cần cho khối Inception ghép đặc trưng


    static tflite::MicroErrorReporter micro_error_reporter;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter, nullptr, nullptr
    );
    
    interpreter = &static_interpreter;
    
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    
    if (allocate_status != kTfLiteOk) {
        Serial.println("LỖI AI NGHIÊM TRỌNG: Khởi tạo mô hình thất bại (AllocateTensors failed)!");
        Serial.println("--> Nguyên nhân có thể do:");
        Serial.println("    - Thiếu toán tử (Operator) trong bộ resolver.");
        Serial.println("    - Thiếu RAM, kích thước kTensorArenaSize quá nhỏ.");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("HỆ THỐNG: Khởi tạo mô hình AI phân loại ECG thành công!");
    Serial.println("HỆ THỐNG: Đang học ngưỡng QRS trong 2 giây đầu (auto-calibration)...");
}

void ECGAIProcessor::setBLEManager(BLEManager* ble) {
    bleManager = ble;
}


void ECGAIProcessor::runInference() {
    // 1. Tinh chỉnh đỉnh R cực trị địa phương trong đệm vòng [-15, +5] xung quanh inference_peak_buffer_idx
    int best_peak_buffer_idx = inference_peak_buffer_idx;
    float max_val_ref = -999999.0f;
    for (int offset = -15; offset <= 5; offset++) {
        int idx = (inference_peak_buffer_idx + offset + BUFFER_SIZE) % BUFFER_SIZE;
        float val = fabsf(ecgBuffer[idx]);
        if (val > max_val_ref) {
            max_val_ref = val;
            best_peak_buffer_idx = idx;
        }
    }

    float temp_buffer[MODEL_INPUT_SIZE];
    float min_val = 999999.0f;
    float max_val = -999999.0f;

    // 2. Trích xuất các mẫu từ Circular Buffer căn lề theo best_peak_buffer_idx làm vị trí 250 và tìm Min/Max
    for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
        int idx = (best_peak_buffer_idx - 250 + i + BUFFER_SIZE) % BUFFER_SIZE;
        temp_buffer[i] = ecgBuffer[idx];
        if (temp_buffer[i] < min_val) min_val = temp_buffer[i];
        if (temp_buffer[i] > max_val) max_val = temp_buffer[i];
    }

    // 2. Tránh lỗi chia 0
    float range = max_val - min_val;
    if (range < 0.0001f) range = 0.0001f;

    // 3. Chuẩn hóa Min-Max [0.0, 1.0] và nạp vào Tensor
    for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
        float normalized_val = (temp_buffer[i] - min_val) / range;
        
        if (input->type == kTfLiteInt8) {
            input->data.int8[i] = (int8_t)(normalized_val / input->params.scale + input->params.zero_point);
        } else {
            input->data.f[i] = normalized_val;
        }
    }

    // 4. Chạy AI
    if (interpreter->Invoke() == kTfLiteOk) {
#if ENABLE_HYBRID_OVERRIDE
        if (hasPacingSpikeInWindow) {
            // Ép kết quả sang Lớp 4 (Paced) do phát hiện gai tạo nhịp phần cứng thô
            finalProtectedResult = 4;
            currentConfidence = 99.0f;
            for (int i = 0; i < NUM_CLASSES; i++) {
                classPercentages[i] = (i == 4) ? 99.0f : 0.0f;
                probHistory[i][probHistoryIdx] = (i == 4) ? 1.0f : 0.0f;
            }
            probHistoryIdx = (probHistoryIdx + 1) % PROB_WINDOW;
            if (bleManager) bleManager->sendDebugMessage("AI_OVERRIDE|Paced beat detected by hardware spike!");
            return;
        }
#endif
        
        char debugMsg[256];
        snprintf(debugMsg, sizeof(debugMsg), "AI_OUTPUT|P0:%.1f|P1:%.1f|P2:%.1f|P3:%.1f|P4:%.1f",
                 output->data.f[0] * 100.0f, output->data.f[1] * 100.0f,
                 output->data.f[2] * 100.0f, output->data.f[3] * 100.0f, output->data.f[4] * 100.0f);
        if (bleManager) bleManager->sendDebugMessage(debugMsg);
        
        // --- BỘ LỌC MƯỢT XÁC SUẤT SMA (SIMPLE MOVING AVERAGE) ---
        // Bước A: Lưu xác suất thô vào lịch sử
        for (int i = 0; i < NUM_CLASSES; i++) {
            probHistory[i][probHistoryIdx] = output->data.f[i];
        }
        probHistoryIdx = (probHistoryIdx + 1) % PROB_WINDOW;

        // Bước B: Tính điểm trung bình trượt của các nhịp gần nhất
        float maxProb = 0;
        int currentClass = 0;
        for (int i = 0; i < NUM_CLASSES; i++) {
            float sum = 0;
            for (int j = 0; j < PROB_WINDOW; j++) {
                sum += probHistory[i][j];
            }
            float avgProb = sum / PROB_WINDOW;
            classPercentages[i] = avgProb * 100.0f;
            
            if (avgProb > maxProb) { 
                maxProb = avgProb; 
                currentClass = i; 
            }
        }
        
        currentConfidence = maxProb * 100.0f;

        if (currentConfidence >= CONFIDENCE_THRESHOLD) {
            finalProtectedResult = currentClass;
        } else {
            finalProtectedResult = 0; // Mặc định về nhịp thường nếu độ tin cậy thấp
        }
    } else {
        Serial.println("LỖI: Hàm Invoke() chạy thất bại!");
    }
}

void ECGAIProcessor::processSample(float raw) {
    peakDetectedFlag = false;

    // 0. [PACEMAKER SPIKE DETECTION]
    // Tính đạo hàm tín hiệu thô để tìm gai tạo nhịp nhọn (độ dốc > 120 đơn vị/mẫu)
#if ENABLE_HYBRID_OVERRIDE
    if (globalSampleCount > 0) {
        float raw_diff = fabs(raw - prevRawSample);
        if (raw_diff > 120.0f) {
            lastSpikeSampleIndex = globalSampleCount;
            if (bleManager) {
                char spikeMsg[64];
                snprintf(spikeMsg, sizeof(spikeMsg), "Spike detected! Diff: %.1f", raw_diff);
                bleManager->sendDebugMessage(spikeMsg);
            }
        }
    }
#endif
    prevRawSample = raw;

    // 1. [NOTCH FILTER 50Hz]
    n_x[0] = raw;
    float notch_out = b0*n_x[0] + b1*n_x[1] + b2*n_x[2] - a1*n_y[1] - a2*n_y[2];
    n_x[2] = n_x[1]; n_x[1] = n_x[0]; n_y[2] = n_y[1]; n_y[1] = notch_out;

    // 2. [BỘ LỌC CHỐNG TRÔI ĐƯỜNG NỀN]
    if (estimated_baseline == 0) estimated_baseline = notch_out; 
    estimated_baseline = (0.01 * notch_out) + (0.99 * estimated_baseline); // Giảm từ 0.05 xuống 0.01 để tránh undershoot méo dạng ST/T
    float wander_removed = notch_out - estimated_baseline;

    // 3. [LÀM MƯỢT CHỐNG NHIỄU CƠ BẮP]
    if (high_freq_smoothed == 0) high_freq_smoothed = wander_removed;
    high_freq_smoothed = (0.8 * wander_removed) + (0.2 * high_freq_smoothed); // Tăng từ 0.4 lên 0.8 để tránh làm tròn/suy hao đỉnh QRS quá mức
    clean_signal = high_freq_smoothed;

    // 4. [BANDPASS FILTER]
    hp_out = 0.95 * (prev_hp + notch_out - prev_raw_n);
    prev_raw_n = notch_out; prev_hp = hp_out;
    lp_out = prev_lp + 0.15 * (hp_out - prev_lp); 
    prev_lp = lp_out;

    // 5. [DIFFERENTIATOR]
    float diff = lp_out - diff_buffer[2];
    diff_buffer[2] = diff_buffer[1]; diff_buffer[1] = lp_out;

    // 6. [|u| & MOVING AVERAGE]
    windowSum -= window[windowIdx];
    window[windowIdx] = fabs(diff);
    windowSum += window[windowIdx];
    windowIdx = (windowIdx + 1) % WINDOW_SIZE;
    mwi_out = windowSum / WINDOW_SIZE;

    // Lưu tín hiệu sạch vào buffer AI
    ecgBuffer[bufferHead] = clean_signal;

    // --- AUTO-CALIBRATION NGƯỠNG QRS ---
    // Trong CALIBRATION_SAMPLES mẫu đầu tiên:
    // Thu thập giá trị MWI lớn nhất (bỏ qua 100 mẫu đầu tránh nhiễu quá độ khởi động bộ lọc), sau đó đặt ngưỡng = 50% peak đó
    if (!calibrationDone) {
        if (globalSampleCount >= 100) {
            if (mwi_out > calibrationMaxMWI) calibrationMaxMWI = mwi_out;
        }
        if (globalSampleCount >= CALIBRATION_SAMPLES) {
            // Kết thúc giai đoạn học: đặt ngưỡng bằng 50% peak MWI
            threshold = calibrationMaxMWI * 0.5f;
            if (threshold < 1.0f) threshold = 1.0f; // Tránh ngưỡng = 0 nếu tín hiệu quá yếu
            calibrationDone = true;
            char calMsg[80];
            snprintf(calMsg, sizeof(calMsg),
                     "Calibration xong! Ngưỡng QRS = %.2f (max MWI = %.2f)", threshold, calibrationMaxMWI);
            Serial.printf("[%lu ms] %s\n", millis(), calMsg);
            if (bleManager) bleManager->sendDebugMessage(calMsg);
        }
        // Trong giai đoạn calibration: không phát hiện đỉnh, chỉ thu thập dữ liệu
        bufferHead = (bufferHead + 1) % BUFFER_SIZE;
        globalSampleCount++;
        return;
    }

    // 7. [AI PENDING QUEUE CHECK]
    if (queue_size > 0 && globalSampleCount >= pending_queue[queue_head].target_count) {
        PendingInference item;
        if (dequeue_inference(item)) {
            if (bleManager) bleManager->sendDebugMessage("Data ready for Inference");
            readyForInference = true;
            inference_peak_buffer_idx = item.peak_buffer_idx;
            inference_peak_global_idx = item.peak_global_idx;
        }
    }

    // 8. [QRS & BPM DETECTION]
    if (mwi_out > threshold) {
        if (globalSampleCount - lastPeakSampleIndex > 40) {
            peakDetectedFlag = true;
            if (bleManager) bleManager->sendDebugMessage("Peak detected");
            
            uint32_t interval = globalSampleCount - lastPeakSampleIndex;
            if (lastPeakSampleIndex > 0) {
                float instantBPM = (60.0 * Fs) / (float)interval;
                
                if (instantBPM > 30 && instantBPM < 220) {
                    if (smoothedBPM == 0) {
                        smoothedBPM = instantBPM;
                        prevSmoothedBPM = instantBPM;
                    } else {
                        // Tính toán tỷ lệ RR để phát hiện tính đều đặn của nhịp
                        float currentRR = (float)interval;
                        float prevRR = (float)(lastPeakSampleIndex - secondLastPeakSampleIndex);
                        if (prevRR > 0) {
                            lastRRRatio = (currentRR < prevRR) ? (currentRR / prevRR) : (prevRR / currentRR);
                        }
                        
                        prevSmoothedBPM = smoothedBPM; // Lưu nhịp tim trước khi cập nhật
                        smoothedBPM = (bpmAlpha * instantBPM) + (1.0f - bpmAlpha) * smoothedBPM;
                    }
                    heartRate = smoothedBPM;
                }
            }
            
            secondLastPeakSampleIndex = lastPeakSampleIndex;
            lastPeakSampleIndex = globalSampleCount;
            threshold = mwi_out * 0.6; 
            
            // Đưa vào hàng đợi suy luận
            enqueue_inference(bufferHead, globalSampleCount, globalSampleCount + SAMPLES_TO_WAIT);
            
            // Thuật toán Safe Scan: Kiểm tra xem gai tạo nhịp có xuất hiện trước đỉnh QRS từ 4 đến 15 mẫu không
            uint32_t samplesSinceSpike = globalSampleCount - lastSpikeSampleIndex;
            hasPacingSpikeInWindow = (samplesSinceSpike >= 4 && samplesSinceSpike <= 15);
        } else {
            if (mwi_out * 0.6 > threshold) {
                threshold = mwi_out * 0.6;
            }
        }
        
    } else {
        threshold *= 0.998; 
    }

    bufferHead = (bufferHead + 1) % BUFFER_SIZE;
    globalSampleCount++;
}

// Getters
float ECGAIProcessor::getBPM() { return heartRate; }
int ECGAIProcessor::getProtectedClass() { return finalProtectedResult; }
float ECGAIProcessor::getConfidence() { return currentConfidence; }
float ECGAIProcessor::getFilteredSignal() { return clean_signal; }
bool ECGAIProcessor::isPeakDetected() { return peakDetectedFlag; }

void ECGAIProcessor::getClassPercentages(float out[5]) {
    for (int i = 0; i < NUM_CLASSES; i++) {
        out[i] = classPercentages[i];
    }
}

void ECGAIProcessor::enqueue_inference(int buffer_idx, uint32_t global_idx, uint32_t target) {
    if (queue_size >= MAX_PENDING_INFERENCES) return; // Tránh tràn hàng đợi
    pending_queue[queue_tail].peak_buffer_idx = buffer_idx;
    pending_queue[queue_tail].peak_global_idx = global_idx;
    pending_queue[queue_tail].target_count = target;
    queue_tail = (queue_tail + 1) % MAX_PENDING_INFERENCES;
    queue_size++;
}

bool ECGAIProcessor::dequeue_inference(PendingInference &out_item) {
    if (queue_size <= 0) return false;
    out_item = pending_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_PENDING_INFERENCES;
    queue_size--;
    return true;
}
