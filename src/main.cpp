#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"
#include "AppUI.h"
#include "CornerGif.h"
#include "SampleGif.h" // Chứa ảnh GIF mẫu 24x24 ngôi sao nhấp nháy
#include "GifRegistry.h" // Chứa danh sách tất cả các ảnh GIF (give_info, happy, idle1, normal, petted, sad, speak, star)

// Khởi tạo các đối tượng quản lý
DisplayManager display(TFT_LED_PIN);
AppUI appUI(display);
CornerGif cornerGif(display); // GIF cố định góc màn hình, phóng to x2 (192x192)





void diagnoseTouchHardware() {
    Serial.println("\n========== KIEM TRA PHAN CUNG CAM UNG ==========");
    
    // 1. Kiem tra muc logic tren GPIO 16 (T_DO / MISO)
    pinMode(16, INPUT_PULLUP);
    delay(10);
    int pUp = digitalRead(16);
    pinMode(16, INPUT_PULLDOWN);
    delay(10);
    int pDown = digitalRead(16);
    
    Serial.printf("[TEST PIN 16] PullUp=%d, PullDown=%d\n", pUp, pDown);
    if (pUp == 0 && pDown == 0) {
        Serial.println(">>> CANH BAO: Chan GPIO 16 dang bi dinh chat vao GND (0V)!");
        Serial.println("    -> Kiem tra xem co cam nham vao chan GND tren board ESP32-S3 khong?");
    } else if (pUp == 1 && pDown == 1) {
        Serial.println(">>> CANH BAO: Chan GPIO 16 dang bi keo cung len 3.3V/5V!");
        Serial.println("    -> Chan SDO cua man hinh LCD co dang cam vao day khong? (Hay rut SDO ra!)");
    } else {
        Serial.println(">>> Chan GPIO 16 hoat dong binh thuong (tha noi tot).");
    }

    // 2. Kiem tra chan T_IRQ (neu duoc cau hinh >= 0)
    if (TOUCH_IRQ_PIN >= 0) {
        pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);
        delay(10);
        int irq = digitalRead(TOUCH_IRQ_PIN);
        Serial.printf("[TEST T_IRQ PIN %d] Logic = %d (%s)\n", 
                      TOUCH_IRQ_PIN, irq, 
                      irq == 0 ? "LOW (Dang co luc an!)" : "HIGH (Khong an hoac chua noi day)");
    } else {
        Serial.println("[TEST T_IRQ] T_IRQ bi vo hieu hoa (-1), su dung che do polling SPI.");
    }
    
    Serial.println("================================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==============================================");
    Serial.println("  ESP32-S3 + ILI9488: Floating GIF Sprite Test ");
    Serial.println("==============================================");

    // Kiem tra chan phan cung
    diagnoseTouchHardware();

    // 1. Khởi tạo màn hình
    Serial.println("[1/4] Khoi tao man hinh TFT...");
    display.init(SCREEN_ROTATION);
    Serial.println("[1/4] Man hinh TFT da khoi tao thanh cong!");

    // 2. Cân chỉnh hoặc nạp dữ liệu cảm ứng
    Serial.println("[2/4] Cau hinh cam ung XPT2046...");
    if (FORCE_CALIBRATION_ON_BOOT) {
        display.runCalibration();
    } else {
        display.setCalibration(DEFAULT_CAL_DATA);
    }
    Serial.println("[2/4] Cam ung da san sang!");

    // 3. Khởi tạo giao diện Dashboard đa tab
    Serial.println("[3/4] Khoi tao giao dien Dashboard UI (4 tabs)...");
    appUI.init();
    appUI.setCornerGif(&cornerGif);

    // 4. Khởi tạo Corner GIF (kích thước 192x192 pixel tại góc trên-phải):
    Serial.println("[4/4] Khoi tao Corner GIF (192x192 pixel tai goc tren phai)...");
    cornerGif.setPosition(278, 40); // Góc trên bên phải (x: 278..470, y: 40..232)
    cornerGif.playByName("normal"); // Mặc định mở biểu cảm "normal"

    Serial.println(">> HE THONG DA SAN SANG! <<");
    Serial.println(">> 4 Tabs: 1) TRANG CHU   2) DO THI 1   3) DO THI 2   4) CAI DAT");
    Serial.println(">> Cham vao linh vat hoac hop loi khuyen de doi loi khuyen & bieu cam!");
}

void loop() {
    // 1. Cập nhật hoạt hình GIF linh vật (khi đang ở Trang Chủ)
    if (appUI.getCurrentView() == VIEW_MAIN) {
        cornerGif.update();
    }

    // 2. Cập nhật giao diện Dashboard, đồ thị và cảm ứng
    appUI.update();

    // 4. Lắng nghe lệnh đổi GIF qua Serial Monitor (gõ tên: happy, sad, normal, speak, next...)
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() > 0) {
            if (cmd.equalsIgnoreCase("next")) {
                cornerGif.next();
            } else if (cmd.equalsIgnoreCase("prev")) {
                cornerGif.prev();
            } else {
                if (!cornerGif.playByName(cmd.c_str())) {
                    Serial.println("Danh sach cac ten GIF hop le:");
                    for (size_t i = 0; i < GIF_REGISTRY_COUNT; i++) {
                        Serial.printf(" - %s\n", GIF_REGISTRY[i].name);
                    }
                }
            }
        }
    }

    delay(2);
}