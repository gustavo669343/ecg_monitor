# 🔌 HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG TOÀN DIỆN (VER 4)
## ESP32-S3 + MÀN HÌNH CẢM ỨNG ILI9488 + AD8232 (ECG) + MAX30102 (SPO2)

Tài liệu này hướng dẫn đấu nối toàn bộ phần cứng cho hệ thống **Máy Giám Sát Điện Tim & Sinh Hiệu Đa Thông Số** hoàn chỉnh.

---

### 1. BẢNG TỔNG HỢP CẮM DÂY (PINOUT TABLE)

| Thiết bị | Chân trên Module | Chân cắm trên ESP32-S3 | Loại tín hiệu & Vai trò |
| :--- | :---: | :---: | :--- |
| **Nguồn chung** | **GND** | **GND** | Nối đất tham chiếu chung (Tất cả thiết bị) |
| | **VCC / 3.3V** | **3V3** | Nguồn 3.3V *(Tuyệt đối không cấp 5V)* |
| **Màn hình TFT LCD** | **SDI (MOSI)** | **GPIO 13** | SPI MOSI (Dữ liệu vẽ từ ESP32 sang màn hình) |
| *(Phần hiển thị hình ảnh)* | **SCK** | **GPIO 12** | SPI SCLK (Xung nhịp chung) |
| | **CS** | **GPIO 10** | Chọn chip LCD (TFT_CS) |
| | **DC / RS** | **GPIO 11** | Chân Lệnh / Dữ liệu (Data/Command) |
| | **RESET (RST)** | **GPIO 9** | Chân Reset màn hình |
| | **LED** | **GPIO 14** *(hoặc 3V3)* | Đèn nền màn hình (Chỉnh độ sáng) |
| | **SDO (MISO)** | **ĐỂ TRỐNG** | **TUYỆT ĐỐI ĐỂ TRỐNG** (Tránh liệt chip cảm ứng) |
| **Cảm ứng XPT2046** | **T_CLK** | **GPIO 12** | Nối chung với chân `SCK` của màn hình |
| *(Phần cảm ứng vuốt chạm)*| **T_DIN** | **GPIO 13** | Nối chung với chân `SDI (MOSI)` của màn hình |
| | **T_DO** | **GPIO 16** | Chân MISO đọc tọa độ chạm về ESP32 |
| | **T_CS** | **GPIO 15** | Chọn chip cảm ứng |
| | **T_IRQ** | **GPIO 47** | Ngắt phát hiện chạm màn hình |
| **Thẻ nhớ SD (MicroSD)** | **SD_CS / MOSI / ...** | **ĐỂ TRỐNG** | Đã nhúng sẵn GIF/Font vào Flash, không cần thẻ SD |
| **Cảm biến AD8232** | **OUTPUT (OUT)** | **GPIO 1** | Analog Input (Kênh ADC1_CH0 an toàn) |
| *(Điện tâm đồ ECG)* | **LO+** | **GPIO 2** | Digital Input (Báo tuột điện cực Phải) |
| | **LO-** | **GPIO 3** | Digital Input (Báo tuột điện cực Trái) |
| **Cảm biến MAX30102**| **SDA** | **GPIO 5** | Dữ liệu I2C Data |
| *(SpO2 & Quang tim)* | **SCL** | **GPIO 6** | Xung nhịp I2C Clock |

> [!WARNING]
> **CẢNH BÁO AN TOÀN ĐIỆN ÁP:**
> * Cả 3 module: **Màn hình ILI9488**, **AD8232**, **MAX30102** đều **BẮT BUỘC** cấp nguồn vào đường **3V3 (3.3V)** của ESP32-S3.
> * **KHÔNG NỐI** vào chân **5V (hoặc VIN)** của ESP32-S3.

---

### 2. VỊ TRÍ ĐO ĐIỆN CỰC ECG AD8232 (Tam giác Einthoven)
1. **Dây Đỏ (RA):** Dán dưới xương đòn bên **Phải** (gần vai Phải).
2. **Dây Vàng (LA):** Dán dưới xương đòn bên **Trái** (gần vai Trái).
3. **Dây Xanh lá (RL):** Dán ở sườn bụng bên **Trái** (điện cực đất triệt tiêu nhiễu).

---

### 3. VỊ TRÍ ĐO MAX30102
* Đặt nhẹ ngón tay trỏ hoặc ngón giữa lên mắt quang học của cảm biến (LED đỏ/hồng ngoại).
* Không ấn quá chặt làm tắc nghẽn lưu thông máu ở mao mạch đầu ngón tay.
