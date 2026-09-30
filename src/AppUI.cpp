#include "AppUI.h"
#include "FontAmi16.h"
#include <math.h>

// Bảng màu chuẩn RGB565 theo hệ thống DESIGN.md
#define CLR_BG          0x0821  // Than chì tối (Dark Graphite)
#define CLR_SURFACE     0x10A2  // Xanh phiến tối (Deep Slate)
#define CLR_SURFACE_HI  0x1948  // Lighter Slate cho tab/nút đang chọn
#define CLR_BORDER      0x2965  // Viền khung xám đá
#define CLR_CYAN        0x07FF  // Neon Cyan rực rỡ
#define CLR_AMBER       0xFDE0  // Hổ phách sáng (Bright Amber)
#define CLR_GREEN       0x07E0  // Neon Emerald
#define CLR_WHITE       0xFFFF  // Trắng tinh khiết
#define CLR_SILVER      0x9CD3  // Bạc lạnh cho phụ đề
#define CLR_YELLOW      0xFFE0  // Vàng tươi
#define CLR_MAGENTA     0xF81F  // Tím hồng
#define CLR_RED         0xF800  // Đỏ cảnh báo

// Cấu trúc lời khuyên song ngữ (Tiếng Việt & English)
struct LocalizedAdvice {
    const char* vi;
    const char* en;
};

// Danh sách lời khuyên y tế & chăm sóc sức khỏe tim mạch từ Ami
static const LocalizedAdvice s_adviceList[] = {
    {
        "Hãy ngồi thả lỏng và thở đều khi đo điện tim ECG nhé!",
        "Stay relaxed and breathe evenly during ECG recording!"
    },
    {
        "Giữ các điện cực dán sát da để tín hiệu tim luôn rõ nét.",
        "Keep electrode pads firmly attached for a clean signal."
    },
    {
        "Đặt ngón tay nhẹ nhàng lên cảm biến để đo SpO2 chuẩn xác.",
        "Rest your finger gently on the sensor for accurate SpO2."
    },
    {
        "Mô hình AI đang theo dõi từng nhịp đập để phát hiện bất thường.",
        "AI model is analyzing each heartbeat to detect arrhythmias."
    },
    {
        "Hạn chế cử động cơ thể khi đang ghi sóng điện tim.",
        "Minimize motion while recording ECG to prevent artifacts."
    },
    {
        "Nhịp tim người trưởng thành khi nghỉ thường từ 60-100 BPM.",
        "Normal resting adult heart rate is between 60-100 BPM."
    },
    {
        "Chỉ số SpO2 trên 95% thể hiện lượng oxy trong máu rất tốt!",
        "SpO2 above 95% indicates healthy blood oxygen levels!"
    },
    {
        "Uống đủ nước và ngủ đủ giấc giúp trái tim luôn khỏe mạnh!",
        "Stay hydrated and sleep well for a strong, healthy heart!"
    },
    {
        "Nếu thấy căng thẳng, hãy hít sâu 4 giây rồi thở ra từ từ.",
        "Feeling stressed? Inhale deeply for 4s, then exhale slowly."
    },
    {
        "Ami chúc bạn và trái tim luôn dồi dào năng lượng tích cực!",
        "Ami wishes you and your heart full of vibrant energy!"
    }
};
static const size_t s_adviceCount = sizeof(s_adviceList) / sizeof(s_adviceList[0]);

AppUI::AppUI(DisplayManager& displayManager)
    : display(displayManager) {
    sweepX = 0;
    lastRawY = -1;
    lastFiltY = -1;
}

void AppUI::init() {
    TFT_eSPI& tft = display.getTft();

    // Tải font chữ mượt mà hỗ trợ tiếng Việt đầy đủ dấu
    tft.loadFont(FontAmi16);

    tft.fillScreen(CLR_BG);

    drawHeader();
    drawNavBar();
    drawMainView();

    lastHeaderUpdate = millis();
    lastAdviceCycle = millis();
    lastMetricsUpdate = millis();
    lastWaveTick = millis();
}

void AppUI::setCornerGif(CornerGif* gif) {
    cornerGif = gif;
    if (cornerGif) {
        cornerGif->setPetTimeoutMs(petTimeoutSec * 1000);
    }
}

void AppUI::pushRealECGSample(float filteredVal) {
    pushRealECGSamples(2048.0f, filteredVal);
}

void AppUI::pushRealECGSamples(float rawVal, float filteredVal) {
    latestRealRaw = rawVal;
    latestRealFilt = filteredVal;
    
    // Chuyển biên độ filteredVal (-500..+1000) thành dải hiển thị 0.3V - 3.0V
    float volt = 1.65f + (filteredVal / 800.0f);
    if (volt > 3.2f) volt = 3.2f;
    if (volt < 0.1f) volt = 0.1f;
    latestRealECG = volt;
}

void AppUI::setBiometrics(float ecgBpm, float spo2, float ppgBpm, int aiClass, const char* aiClassName) {
    realEcgBpm = ecgBpm;
    realSpO2 = spo2;
    realPpgBpm = ppgBpm;
    realAiClass = aiClass;
    if (aiClassName) {
        realAiClassName = String(aiClassName);
    }
}

void AppUI::setLanguage(AppLanguage lang) {
    if (currentLang == lang) return;
    currentLang = lang;

    // Vẽ lại toàn bộ giao diện theo ngôn ngữ mới
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(0, 36, 480, 244, CLR_BG);
    drawHeader();
    drawNavBar();

    switch (currentView) {
        case VIEW_MAIN:     drawMainView(); break;
        case VIEW_GRAPH1:   drawGraph1View(); break;
        case VIEW_SETTINGS: drawSettingsView(); break;
    }
}

void AppUI::toggleLanguage() {
    setLanguage((currentLang == LANG_VI) ? LANG_EN : LANG_VI);
}

void AppUI::switchView(AppView view) {
    if (currentView == view) return;
    currentView = view;

    TFT_eSPI& tft = display.getTft();
    tft.fillRect(0, 36, 480, 244, CLR_BG);
    drawHeader();
    drawNavBar();

    switch (currentView) {
        case VIEW_MAIN:
            drawMainView();
            break;
        case VIEW_GRAPH1:
            drawGraph1View();
            break;
        case VIEW_SETTINGS:
            drawSettingsView();
            break;
    }
}

// ==========================================
// HEADER & THANH ĐIỀU HƯỚNG
// ==========================================
void AppUI::drawHeader() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(0, 0, 480, 36, CLR_SURFACE);
    tft.drawFastHLine(0, 35, 480, CLR_BORDER);

    // Đèn LED trạng thái xanh lá
    tft.fillCircle(16, 18, 5, CLR_GREEN);
    tft.drawCircle(16, 18, 6, CLR_WHITE);

    // Tiêu đề phân hệ
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);

    const char* title = "ESP32-S3 COMPANION";
    if (currentView == VIEW_MAIN)     title = tr("TRANG CHỦ (DASHBOARD)", "HOME DASHBOARD");
    if (currentView == VIEW_GRAPH1)   title = tr("ĐỒ THỊ ECG (THỜI GIAN THỰC)", "ECG GRAPH (REAL-TIME)");
    if (currentView == VIEW_SETTINGS) title = tr("CÀI ĐẶT HỆ THỐNG", "SYSTEM SETTINGS");

    tft.drawString(title, 28, 18);

    // Nút chuyển đổi nhanh ngôn ngữ góc trên phải (x: 430, y: 6, w: 42, h: 24)
    tft.fillRoundRect(430, 6, 42, 24, 4, CLR_SURFACE_HI);
    tft.drawRoundRect(430, 6, 42, 24, 4, CLR_CYAN);
    tft.setTextColor(CLR_YELLOW, CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString((currentLang == LANG_VI) ? "VI" : "EN", 451, 18);

    updateHeaderStats();
}

void AppUI::updateHeaderStats() {
    TFT_eSPI& tft = display.getTft();
    // Xóa khoảng trống giữa tiêu đề và nút ngôn ngữ
    tft.fillRect(265, 5, 160, 26, CLR_SURFACE);

    tft.setTextColor(CLR_CYAN, CLR_SURFACE);
    tft.setTextDatum(MR_DATUM);

    uint32_t sec = millis() / 1000;
    uint32_t m = (sec / 60) % 60;
    uint32_t s = sec % 60;

    char buf[40];
    if (cornerGif) {
        snprintf(buf, sizeof(buf), "[Ami: %s] %02u:%02u", cornerGif->getCurrentName(), (unsigned int)m, (unsigned int)s);
    } else {
        snprintf(buf, sizeof(buf), "[Ami] %02u:%02u", (unsigned int)m, (unsigned int)s);
    }
    tft.drawString(buf, 422, 18);
}

void AppUI::drawNavBar() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(0, 280, 480, 40, CLR_SURFACE);
    tft.drawFastHLine(0, 279, 480, CLR_BORDER);

    const char* const tabs_vi[] = { "TRANG CHỦ", "ĐỒ THỊ ECG", "CÀI ĐẶT" };
    const char* const tabs_en[] = { "HOME", "ECG GRAPH", "SETTINGS" };
    const char* const* tabs = (currentLang == LANG_VI) ? tabs_vi : tabs_en;
    const uint16_t tabW = 160;

    for (uint8_t i = 0; i < 3; i++) {
        uint16_t tx = i * tabW;
        bool isActive = (currentView == (AppView)i);

        if (isActive) {
            tft.fillRect(tx, 280, tabW, 40, CLR_SURFACE_HI);
            tft.fillRect(tx + 4, 280, tabW - 8, 3, CLR_CYAN);
            tft.setTextColor(CLR_WHITE, CLR_SURFACE_HI);
        } else {
            tft.setTextColor(CLR_SILVER, CLR_SURFACE);
        }

        tft.setTextDatum(MC_DATUM);
        tft.drawString(tabs[i], tx + tabW / 2, 300);

        if (i > 0) {
            tft.drawFastVLine(tx, 285, 30, CLR_BORDER);
        }
    }
}

// ==========================================
// VIEW 0: TRANG CHỦ & LINH VẬT LỜI KHUYÊN AMI
// ==========================================
void AppUI::drawMainView() {
    TFT_eSPI& tft = display.getTft();

    // 1. Khung lời khuyên từ Ami
    drawAdviceBubble();

    // 2. Hai thẻ thông số y sinh thời gian thực
    // Thẻ 1: Điện tim ECG & Chẩn đoán AI (x: 10, y: 164, w: 125, h: 108)
    tft.fillRoundRect(10, 164, 125, 108, 8, CLR_SURFACE);
    tft.drawRoundRect(10, 164, 125, 108, 8, CLR_BORDER);
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(tr("ĐIỆN TIM & AI", "ECG & AI"), 72, 172);

    // Thẻ 2: Cảm biến SpO2 & Quang tim (x: 145, y: 164, w: 125, h: 108)
    tft.fillRoundRect(145, 164, 125, 108, 8, CLR_SURFACE);
    tft.drawRoundRect(145, 164, 125, 108, 8, CLR_BORDER);
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(tr("OXY & MẠCH", "SPO2 & PULSE"), 207, 172);

    // Cập nhật thông số thực tế
    updateRealMetrics();

    // 3. Linh vật Ami ở góc trên bên phải
    if (cornerGif) {
        cornerGif->setPosition(278, 40);
        cornerGif->redraw();
    }
}

void AppUI::updateRealMetrics() {
    if (currentView != VIEW_MAIN) return;

    TFT_eSPI& tft = display.getTft();
    char buf[32];

    // --- THẺ 1: ECG BPM & CHẨN ĐOÁN AI ---
    tft.fillRect(12, 194, 121, 26, CLR_SURFACE);
    tft.setTextColor(CLR_RED, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    if (realEcgBpm > 0) {
        snprintf(buf, sizeof(buf), "%.0f BPM", realEcgBpm);
    } else {
        snprintf(buf, sizeof(buf), "-- BPM");
    }
    tft.drawString(buf, 72, 196);

    tft.fillRect(12, 240, 121, 26, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    if (!aiEnabled) {
        tft.setTextColor(CLR_SILVER, CLR_SURFACE);
        tft.drawString(tr("[ AI: ĐÃ TẮT ]", "[ AI: OFF ]"), 72, 246);
    } else {
        tft.setTextColor(realAiClass == 0 ? CLR_GREEN : (realAiClass == 2 ? CLR_RED : CLR_AMBER), CLR_SURFACE);
        if (realAiClass == 0) {
            snprintf(buf, sizeof(buf), tr("Bình thường (N)", "Normal (N)"));
        } else if (realAiClass == 2) {
            snprintf(buf, sizeof(buf), tr("Ngoại tâm (V)", "PVC Beat (V)"));
        } else if (realAiClass == 1) {
            snprintf(buf, sizeof(buf), tr("Trên thất (S)", "Supravent (S)"));
        } else if (realAiClass == 3) {
            snprintf(buf, sizeof(buf), tr("Hỗn hợp (F)", "Fusion (F)"));
        } else {
            snprintf(buf, sizeof(buf), "%s", realAiClassName.c_str());
        }
        tft.drawString(buf, 72, 246);
    }

    // --- THẺ 2: SPO2 & PPG PULSE RATE ---
    tft.fillRect(147, 194, 121, 26, CLR_SURFACE);
    tft.setTextColor(CLR_CYAN, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    if (realSpO2 > 0) {
        snprintf(buf, sizeof(buf), "%.0f%% SpO2", realSpO2);
    } else {
        snprintf(buf, sizeof(buf), "--%% SpO2");
    }
    tft.drawString(buf, 207, 196);

    tft.fillRect(147, 240, 121, 26, CLR_SURFACE);
    tft.setTextColor(CLR_AMBER, CLR_SURFACE);
    tft.setTextDatum(TC_DATUM);
    if (realPpgBpm > 0) {
        snprintf(buf, sizeof(buf), "Mạch: %.0f", realPpgBpm);
    } else {
        snprintf(buf, sizeof(buf), "Mạch: --");
    }
    tft.drawString(buf, 207, 246);
}

void AppUI::drawAdviceBubble(const char* customText) {
    TFT_eSPI& tft = display.getTft();

    const int bx = 10;
    const int by = 42;
    const int bw = 258;
    const int bh = 114;

    tft.fillRoundRect(bx, by, bw, bh, 8, CLR_SURFACE);
    tft.drawRoundRect(bx, by, bw, bh, 8, CLR_CYAN);

    // Đuôi bóng thoại trỏ về Ami
    tft.fillTriangle(bx + bw - 2, by + 40, bx + bw + 10, by + 50, bx + bw - 2, by + 60, CLR_SURFACE);
    tft.drawLine(bx + bw - 2, by + 40, bx + bw + 10, by + 50, CLR_CYAN);
    tft.drawLine(bx + bw + 10, by + 50, bx + bw - 2, by + 60, CLR_CYAN);

    // Tiêu đề
    tft.setTextColor(CLR_AMBER, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    const char* headerText = customText 
        ? tr("✦ PHẢN HỒI TỪ AMI", "✦ AMI'S REACTION")
        : tr("✦ LỜI KHUYÊN TỪ AMI", "✦ AMI'S ADVICE");
    tft.drawString(headerText, bx + 12, by + 8);
    tft.drawFastHLine(bx + 10, by + 29, bw - 20, CLR_BORDER);

    // Nội dung lời khuyên
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    const char* text = customText 
        ? customText 
        : (currentLang == LANG_VI ? s_adviceList[currentAdviceIdx].vi : s_adviceList[currentAdviceIdx].en);

    // Cắt dòng thông minh theo pixel width (tối đa 236px)
    const int maxLineWidth = bw - 22;
    char line1[80] = {0};
    char line2[80] = {0};
    char line3[80] = {0};

    const char* p = text;
    char word[48];
    uint8_t lineNum = 1;
    char curLine[128] = {0};

    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        uint8_t wi = 0;
        while (*p && *p != ' ' && wi < sizeof(word) - 1) {
            word[wi++] = *p++;
        }
        word[wi] = '\0';

        char testLine[128];
        if (curLine[0] == '\0') {
            snprintf(testLine, sizeof(testLine), "%s", word);
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", curLine, word);
        }

        if (tft.textWidth(testLine) <= maxLineWidth) {
            strncpy(curLine, testLine, sizeof(curLine) - 1);
        } else {
            if (lineNum == 1) {
                strncpy(line1, curLine, sizeof(line1) - 1);
                lineNum = 2;
                strncpy(curLine, word, sizeof(curLine) - 1);
            } else if (lineNum == 2) {
                strncpy(line2, curLine, sizeof(line2) - 1);
                lineNum = 3;
                strncpy(curLine, word, sizeof(curLine) - 1);
            } else {
                break;
            }
        }
    }

    if (lineNum == 1) strncpy(line1, curLine, sizeof(line1) - 1);
    else if (lineNum == 2) strncpy(line2, curLine, sizeof(line2) - 1);
    else if (lineNum == 3) strncpy(line3, curLine, sizeof(line3) - 1);

    tft.drawString(line1, bx + 12, by + 35);
    if (line2[0]) tft.drawString(line2, bx + 12, by + 57);
    if (line3[0]) tft.drawString(line3, bx + 12, by + 79);

    // Gợi ý tương tác ở đáy hộp
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    const char* hintText = tr("(Chạm hộp đổi lời • Vuốt Ami để chơi)", "(Tap box to cycle • Pet Ami to play)");
    tft.drawString(hintText, bx + 12, by + (line3[0] ? 97 : 89));
}

void AppUI::nextAdvice() {
    currentAdviceIdx = (currentAdviceIdx + 1) % s_adviceCount;
    lastAdviceCycle = millis();

    // Khi người dùng chạm hộp thoại để đổi lời khuyên, Ami nói chuyện 3s rồi về normal
    if (cornerGif) {
        cornerGif->playByName("speak");
        cornerGif->setTimeout(3000, "normal");
    }

    drawAdviceBubble();
}

// ==========================================
// VIEW 1: ĐỒ THỊ 1 (PLOT GRAPH 1 - 2 TẦNG CHUẨN PYTHON GUI)
// ==========================================
void AppUI::drawGraph1View() {
    TFT_eSPI& tft = display.getTft();

    // Reset trạng thái vệt quét
    sweepX = 0;
    lastRawY = -1;
    lastFiltY = -1;
    lastGraphTextUpdate = 0;

    // ========================================================
    // TẦNG 1 (TRÊN): TÍN HIỆU ĐIỆN TIM THÔ (RAW AD8232) - MÀU ĐỎ
    // ========================================================
    const int c1_x = 10, c1_y = 38, c1_w = 460, c1_h = 114;
    tft.fillRoundRect(c1_x, c1_y, c1_w, c1_h, 6, CLR_SURFACE);
    tft.drawRoundRect(c1_x, c1_y, c1_w, c1_h, 6, CLR_BORDER);

    // Tiêu đề kênh thô (Chữ đỏ như Python GUI)
    tft.setTextColor(TFT_RED, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tr("TÍN HIỆU THÔ (RAW AD8232)", "RAW SIGNAL (AD8232)"), c1_x + 10, c1_y + 4);

    // Vùng vẽ sóng thô (Kích thước: 415 x 86)
    const int p1_x = PLOT_X, p1_y = 58, p1_w = PLOT_W, p1_h = 86;
    tft.fillRect(p1_x, p1_y, p1_w, p1_h, TFT_BLACK);
    tft.drawRect(p1_x - 1, p1_y - 1, p1_w + 2, p1_h + 2, CLR_BORDER);

    // Lưới toạ độ kênh thô (Màu xám tối)
    for (int y = p1_y + 28; y < p1_y + p1_h; y += 28) {
        for (int x = p1_x; x < p1_x + p1_w; x += 10) {
            tft.drawPixel(x, y, 0x2104);
        }
    }
    for (int x = p1_x + 50; x < p1_x + p1_w; x += 50) {
        for (int y = p1_y; y < p1_y + p1_h; y += 8) {
            tft.drawPixel(x, y, 0x2104);
        }
    }

    // Nhãn trục Y kênh thô (0..4095)
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(MR_DATUM);
    tft.drawString("4K", p1_x - 4, p1_y + 4);
    tft.drawString("2K", p1_x - 4, p1_y + 43);
    tft.drawString("0", p1_x - 4, p1_y + 82);

    // ========================================================
    // TẦNG 2 (DƯỚI): TÍN HIỆU SẠCH SAU LỌC NOTCH 50Hz - MÀU XANH LÁ
    // ========================================================
    const int c2_x = 10, c2_y = 156, c2_w = 460, c2_h = 118;
    tft.fillRoundRect(c2_x, c2_y, c2_w, c2_h, 6, CLR_SURFACE);
    tft.drawRoundRect(c2_x, c2_y, c2_w, c2_h, 6, CLR_BORDER);

    // Tiêu đề kênh sạch (Chữ xanh lá như Python GUI)
    tft.setTextColor(TFT_GREEN, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tr("TÍN HIỆU SẠCH (NOTCH 50Hz)", "CLEAN SIGNAL (50Hz NOTCH)"), c2_x + 10, c2_y + 4);

    // Vùng vẽ sóng sạch (Kích thước: 415 x 88)
    const int p2_x = PLOT_X, p2_y = 178, p2_w = PLOT_W, p2_h = 88;
    tft.fillRect(p2_x, p2_y, p2_w, p2_h, TFT_BLACK);
    tft.drawRect(p2_x - 1, p2_y - 1, p2_w + 2, p2_h + 2, CLR_BORDER);

    // Lưới toạ độ kênh sạch
    for (int y = p2_y + 29; y < p2_y + p2_h; y += 29) {
        for (int x = p2_x; x < p2_x + p2_w; x += 10) {
            tft.drawPixel(x, y, 0x2104);
        }
    }
    for (int x = p2_x + 50; x < p2_x + p2_w; x += 50) {
        for (int y = p2_y; y < p2_y + p2_h; y += 8) {
            tft.drawPixel(x, y, 0x2104);
        }
    }

    // Nhãn trục Y kênh sạch (-500..+1000)
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(MR_DATUM);
    tft.drawString("+1K", p2_x - 4, p2_y + 4);
    tft.drawString("0", p2_x - 4, p2_y + 44);
    tft.drawString("-500", p2_x - 4, p2_y + 84);
}

void AppUI::updateGraph1Wave() {
    if (wavePaused || currentView != VIEW_GRAPH1) return;

    TFT_eSPI& tft = display.getTft();
    const int p1_x = PLOT_X, p1_y = 58, p1_w = PLOT_W, p1_h = 86;
    const int p2_x = PLOT_X, p2_y = 178, p2_w = PLOT_W, p2_h = 88;

    // Tọa độ quét hiện tại
    int curX = p1_x + sweepX;
    int clearX = p1_x + ((sweepX + 1) % p1_w);

    // 1. Xóa vệt 6 pixel phía trước đầu quét (Sweep Erase) để vẽ nét mới mượt mà
    tft.fillRect(clearX, p1_y, 6, p1_h, TFT_BLACK);
    tft.fillRect(clearX, p2_y, 6, p2_h, TFT_BLACK);

    // Khôi phục lưới nếu vệt xóa trùng tọa độ lưới
    if (clearX % 50 <= 5) {
        int gx = clearX - (clearX % 50) + 50;
        if (gx >= clearX && gx < clearX + 6 && gx < p1_x + p1_w) {
            for (int y = p1_y; y < p1_y + p1_h; y += 8) tft.drawPixel(gx, y, 0x2104);
            for (int y = p2_y; y < p2_y + p2_h; y += 8) tft.drawPixel(gx, y, 0x2104);
        }
    }

    // 2. Tính toán tọa độ Y cho TÍN HIỆU THÔ (0..4095 -> p1_y..p1_y+p1_h)
    float rawVal = latestRealRaw;
    int rawY = p1_y + p1_h - 2 - (int)((rawVal / 4095.0f) * (p1_h - 4));
    rawY = constrain(rawY, p1_y + 2, p1_y + p1_h - 2);

    // 3. Tính toán tọa độ Y cho TÍN HIỆU SẠCH (-500..+1000, 0 nằm ở giữa)
    float filtVal = latestRealFilt;
    int filtY = p2_y + (p2_h / 2) - (int)((filtVal / 600.0f) * 36.0f);
    filtY = constrain(filtY, p2_y + 2, p2_y + p2_h - 2);

    // 4. Vẽ đoạn thẳng nối giữa điểm trước và điểm hiện tại
    if (lastRawY > 0 && sweepX > 0) {
        int prevX = p1_x + sweepX - 1;
        // Kênh Thô: Màu Đỏ rực
        tft.drawLine(prevX, lastRawY, curX, rawY, TFT_RED);
        // Kênh Sạch: Màu Xanh lá cây y tế (Vẽ 2 nét cho đậm rõ)
        tft.drawLine(prevX, lastFiltY, curX, filtY, TFT_GREEN);
        tft.drawLine(prevX, lastFiltY + 1, curX, filtY + 1, TFT_GREEN);
    }

    lastRawY = rawY;
    lastFiltY = filtY;
    sweepX = (sweepX + 1) % p1_w;

    // 5. Cập nhật các nhãn số đo tức thời ở góc phải tiêu đề mỗi 400ms
    uint32_t now = millis();
    if (now - lastGraphTextUpdate > 400) {
        lastGraphTextUpdate = now;
        char buf[40];

        // Nhãn Raw tức thời
        tft.fillRect(350, 41, 115, 14, CLR_SURFACE);
        tft.setTextColor(TFT_RED, CLR_SURFACE);
        tft.setTextDatum(TR_DATUM);
        snprintf(buf, sizeof(buf), "Raw: %.0f", rawVal);
        tft.drawString(buf, 465, 41);

        // Nhãn Sinh hiệu (BPM & SpO2)
        tft.fillRect(290, 159, 175, 14, CLR_SURFACE);
        tft.setTextColor(TFT_GREEN, CLR_SURFACE);
        tft.setTextDatum(TR_DATUM);
        if (realEcgBpm > 0) {
            snprintf(buf, sizeof(buf), "BPM: %.0f | SpO2: %.0f%%", realEcgBpm, realSpO2 > 0 ? realSpO2 : 98.0f);
        } else {
            snprintf(buf, sizeof(buf), "-- BPM | SpO2: --%%");
        }
        tft.drawString(buf, 465, 159);
    }
}

// ==========================================
// VIEW 2: CÀI ĐẶT HỆ THỐNG & AI (SETTINGS)
// ==========================================
void AppUI::drawSettingsView() {
    TFT_eSPI& tft = display.getTft();

    // Hàng 1: Ngôn ngữ / Language (y: 40..74)
    tft.fillRoundRect(15, 40, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 40, 450, 34, 6, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Ngôn ngữ / Language:", "Language / Ngôn ngữ:"), 25, 57);

    // Nút TIẾNG VIỆT
    bool isVI = (currentLang == LANG_VI);
    tft.fillRoundRect(210, 43, 110, 28, 4, isVI ? CLR_CYAN : CLR_SURFACE_HI);
    tft.drawRoundRect(210, 43, 110, 28, 4, isVI ? CLR_WHITE : CLR_BORDER);
    tft.setTextColor(isVI ? TFT_BLACK : CLR_SILVER, isVI ? CLR_CYAN : CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("TIẾNG VIỆT", 265, 57);

    // Nút ENGLISH
    bool isEN = (currentLang == LANG_EN);
    tft.fillRoundRect(330, 43, 110, 28, 4, isEN ? CLR_CYAN : CLR_SURFACE_HI);
    tft.drawRoundRect(330, 43, 110, 28, 4, isEN ? CLR_WHITE : CLR_BORDER);
    tft.setTextColor(isEN ? TFT_BLACK : CLR_SILVER, isEN ? CLR_CYAN : CLR_SURFACE_HI);
    tft.drawString("ENGLISH", 385, 57);

    // Hàng 2: Độ sáng màn hình (y: 78..112)
    tft.fillRoundRect(15, 78, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 78, 450, 34, 6, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Độ sáng màn hình:", "Screen Brightness:"), 25, 95);

    tft.fillRoundRect(210, 81, 34, 28, 4, CLR_SURFACE_HI);
    tft.drawRoundRect(210, 81, 34, 28, 4, CLR_BORDER);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("-", 227, 95);

    for (uint8_t i = 1; i <= 5; i++) {
        uint16_t col = (i <= brightnessStep) ? CLR_CYAN : CLR_BORDER;
        tft.fillRoundRect(252 + (i - 1) * 22, 86, 18, 18, 2, col);
    }

    tft.fillRoundRect(370, 81, 34, 28, 4, CLR_SURFACE_HI);
    tft.drawRoundRect(370, 81, 34, 28, 4, CLR_BORDER);
    tft.drawString("+", 387, 95);

    // Hàng 3: Mô hình AI dự đoán nhịp tim (y: 116..150)
    tft.fillRoundRect(15, 116, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 116, 450, 34, 6, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Mô hình AI nhịp tim:", "ECG AI Model:"), 25, 133);

    tft.fillRoundRect(310, 119, 130, 28, 4, aiEnabled ? CLR_GREEN : CLR_SURFACE_HI);
    tft.drawRoundRect(310, 119, 130, 28, 4, CLR_WHITE);
    tft.setTextColor(aiEnabled ? TFT_BLACK : CLR_SILVER, aiEnabled ? CLR_GREEN : CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(aiEnabled ? tr("BẬT (ON)", "ON") : tr("TẮT (OFF)", "OFF"), 375, 133);

    // Hàng 4: Thời gian vuốt ve Ami (y: 154..188)
    tft.fillRoundRect(15, 154, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 154, 450, 34, 6, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Thời gian vuốt ve Ami:", "Ami Pet Timeout:"), 25, 171);

    const uint8_t petOpts[] = { 4, 6, 10 };
    for (uint8_t i = 0; i < 3; i++) {
        int bx = 240 + i * 68;
        bool isSel = (petTimeoutSec == petOpts[i]);
        tft.fillRoundRect(bx, 157, 60, 28, 4, isSel ? CLR_CYAN : CLR_SURFACE_HI);
        tft.drawRoundRect(bx, 157, 60, 28, 4, isSel ? CLR_WHITE : CLR_BORDER);
        tft.setTextColor(isSel ? TFT_BLACK : CLR_WHITE, isSel ? CLR_CYAN : CLR_SURFACE_HI);
        tft.setTextDatum(MC_DATUM);
        char pBuf[12];
        snprintf(pBuf, sizeof(pBuf), "%us", petOpts[i]);
        tft.drawString(pBuf, bx + 30, 171);
    }

    // Hàng 5: Tự động đổi lời khuyên (y: 192..226)
    tft.fillRoundRect(15, 192, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 192, 450, 34, 6, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Tự đổi lời khuyên (8s):", "Auto-Cycle Advice:"), 25, 209);

    tft.fillRoundRect(310, 195, 130, 28, 4, autoCycleAdvice ? CLR_GREEN : CLR_SURFACE_HI);
    tft.drawRoundRect(310, 195, 130, 28, 4, CLR_WHITE);
    tft.setTextColor(autoCycleAdvice ? TFT_BLACK : CLR_SILVER, autoCycleAdvice ? CLR_GREEN : CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(autoCycleAdvice ? tr("BẬT (ON)", "ON") : tr("TẮT (OFF)", "OFF"), 375, 209);

    // Hàng 6: Nút Cân chỉnh cảm ứng (y: 232..272)
    tft.fillRoundRect(40, 232, 400, 40, 6, CLR_SURFACE_HI);
    tft.drawRoundRect(40, 232, 400, 40, 6, CLR_CYAN);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(tr("✦ CÂN CHỈNH LẠI CẢM ỨNG (CALIB) ✦", "✦ RE-CALIBRATE TOUCH SCREEN ✦"), 240, 252);
}

void AppUI::applyBrightness(uint8_t step) {
    brightnessStep = constrain(step, 1, 5);
    display.setBrightness(brightnessStep * 50);
    if (currentView == VIEW_SETTINGS) {
        drawSettingsView();
    }
}

// ==========================================
// CẬP NHẬT TRẠNG THÁI & CẢM ỨNG
// ==========================================
void AppUI::update() {
    uint32_t now = millis();

    // 1. Tự động xoay vòng lời khuyên mỗi 8 giây trên Trang Chủ (nếu không đang hiện phản hồi vuốt ve)
    if (autoCycleAdvice && currentView == VIEW_MAIN && !showingPetReaction) {
        if (now - lastAdviceCycle >= 8000) {
            nextAdvice();
        }
    }

    // Nếu đang hiện phản hồi khi được pet, kiểm tra khi linh vật hết timeout quay về normal thì trả lại lời khuyên bình thường
    if (showingPetReaction && currentView == VIEW_MAIN && cornerGif) {
        if (strcasecmp(cornerGif->getCurrentName(), "normal") == 0) {
            showingPetReaction = false;
            drawAdviceBubble();
        }
    }

    // 2. Cập nhật đồ thị thời gian thực nếu đang ở View 1
    if (currentView == VIEW_GRAPH1) {
        updateGraph1Wave();
    }

    // 3. Cập nhật thông số phần cứng định kỳ mỗi 2 giây ở Trang Chủ
    if (currentView == VIEW_MAIN && (now - lastMetricsUpdate >= 2000)) {
        lastMetricsUpdate = now;
        updateRealMetrics();
    }

    // 4. Cập nhật đồng hồ và header mỗi 1 giây
    if (now - lastHeaderUpdate >= 1000) {
        lastHeaderUpdate = now;
        updateHeaderStats();
    }

    // 5. Xử lý cảm ứng
    uint16_t tx = 0, ty = 0;
    if (display.getTouch(&tx, &ty)) {
        if (now - lastTouchTime >= 180) {
            lastTouchTime = now;
            handleTouch(tx, ty);
        }
    }
}

void AppUI::handleTouch(uint16_t tx, uint16_t ty) {
    // 0. Nút chuyển đổi nhanh ngôn ngữ trên Header (x: 420..478, y: 0..36)
    if (ty <= 36 && tx >= 420) {
        toggleLanguage();
        return;
    }

    // A. Chạm vào thanh Navigation Menu ở đáy màn hình (Y: 278..320, 3 Tabs x 160px)
    if (ty >= 278) {
        if (tx < 160) {
            switchView(VIEW_MAIN);
        } else if (tx < 320) {
            switchView(VIEW_GRAPH1);
        } else {
            switchView(VIEW_SETTINGS);
        }
        return;
    }

    // B. Xử lý chạm theo từng View cụ thể
    if (currentView == VIEW_MAIN) {
        // B1. Chạm vào vùng linh vật Ami ở góc trên phải: x: 278..470, y: 40..232
        if (cornerGif && cornerGif->contains(tx, ty)) {
            if (ty < 135) {
                // CHẠM ĐẦU -> XOA ĐẦU (PETTED): timeout kéo dài đúng số giây đã cài đặt!
                if (strcasecmp(cornerGif->getCurrentName(), "petted") == 0) {
                    cornerGif->refreshTimeout();
                } else {
                    cornerGif->playByName("petted");
                }
                showingPetReaction = true;
                petReactionStartTime = millis();
                drawAdviceBubble(tr(
                    "Oa, được bạn xoa đầu Ami thích quá! Cảm ơn bạn nhiều nhé! <3",
                    "Yay, thank you for petting Ami! I love this so much! <3"
                ));
            } else {
                // CHẠM MÁ/THÂN -> VUI VẺ (HAPPY)
                cornerGif->playByName("happy");
                cornerGif->setTimeout(4000, "normal");
                showingPetReaction = true;
                petReactionStartTime = millis();
                drawAdviceBubble(tr(
                    "Hehe nhột quá! Ami chúc bạn một ngày thật nhiều năng lượng!",
                    "Hehe ticklish! Ami wishes you lots of energy today!"
                ));
            }
            return;
        }

        // B2. Chạm vào hộp lời khuyên: Đổi sang lời khuyên kế tiếp & Ami nói chuyện
        if (tx >= 10 && tx <= 268 && ty >= 42 && ty <= 156) {
            showingPetReaction = false;
            nextAdvice();
            return;
        }
    }
    else if (currentView == VIEW_GRAPH1) {
        // Chạm vào vùng đồ thị để Tạm dừng / Tiếp tục quét sóng (Pause/Resume)
        if (ty >= 38 && ty <= 274) {
            wavePaused = !wavePaused;
            return;
        }
    }
    else if (currentView == VIEW_SETTINGS) {
        // Hàng 1: Nút chọn ngôn ngữ (y: 40..74)
        if (ty >= 40 && ty <= 74) {
            if (tx >= 210 && tx <= 320) {
                setLanguage(LANG_VI);
                return;
            }
            if (tx >= 330 && tx <= 440) {
                setLanguage(LANG_EN);
                return;
            }
        }

        // Hàng 2: Nút [-] và [+] độ sáng (y: 78..112)
        if (ty >= 78 && ty <= 112) {
            if (tx >= 200 && tx <= 248) {
                if (brightnessStep > 1) applyBrightness(brightnessStep - 1);
                return;
            }
            if (tx >= 360 && tx <= 410) {
                if (brightnessStep < 5) applyBrightness(brightnessStep + 1);
                return;
            }
        }

        // Hàng 3: Nút Bật/Tắt Mô hình AI nhịp tim (y: 116..150)
        if (ty >= 116 && ty <= 150) {
            if (tx >= 300 && tx <= 445) {
                aiEnabled = !aiEnabled;
                drawSettingsView();
                return;
            }
        }

        // Hàng 4: Chọn thời gian Pet Timeout: 4s, 6s, 10s (y: 154..188)
        if (ty >= 154 && ty <= 188) {
            const uint8_t petOpts[] = { 4, 6, 10 };
            for (uint8_t i = 0; i < 3; i++) {
                int bx = 240 + i * 68;
                if (tx >= bx && tx <= bx + 60) {
                    petTimeoutSec = petOpts[i];
                    if (cornerGif) cornerGif->setPetTimeoutMs(petTimeoutSec * 1000);
                    drawSettingsView();
                    return;
                }
            }
        }

        // Hàng 5: Nút Bật/Tắt xoay lời khuyên (y: 192..226)
        if (ty >= 192 && ty <= 226) {
            if (tx >= 300 && tx <= 445) {
                autoCycleAdvice = !autoCycleAdvice;
                drawSettingsView();
                return;
            }
        }

        // Hàng 6: Nút Cân chỉnh cảm ứng (y: 232..272)
        if (ty >= 232 && ty <= 272) {
            if (tx >= 40 && tx <= 440) {
                display.runCalibration();
                switchView(VIEW_SETTINGS);
                return;
            }
        }
    }
}
