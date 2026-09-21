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

// Danh sách lời khuyên phong phú từ Ami
static const LocalizedAdvice s_adviceList[] = {
    {
        "ESP32-S3 đã sẵn sàng! Hệ thống vận hành rất mượt mà.",
        "ESP32-S3 is online! System is running smooth and stable."
    },
    {
        "Đồ thị 1 hỗ trợ 3 sóng: Sine, Nhịp tim và Tam giác.",
        "Graph 1 supports 3 waves: Sine, Heartbeat, and Triangle."
    },
    {
        "Đồ thị 2 so sánh 6 kênh đo. Chạm cột xem chi tiết!",
        "Graph 2 tracks 6 channels. Tap any bar for details!"
    },
    {
        "RAM dồi dào, chip lõi kép Xtensa xử lý siêu tốc!",
        "RAM is plentiful, Xtensa dual-core is blazingly fast!"
    },
    {
        "Bạn có thể xoa đầu hoặc vuốt má để chơi cùng Ami nhé!",
        "Stroke Ami's head or tap cheeks to play together!"
    },
    {
        "Vào menu Cài đặt để tùy chỉnh thời lượng vuốt ve Ami.",
        "Visit Settings to customize Ami's petting duration."
    },
    {
        "Độ sáng màn hình có 5 mức giúp bảo vệ đôi mắt bạn.",
        "Screen brightness has 5 levels to protect your eyes."
    },
    {
        "Bạn đã làm việc chăm chỉ, hãy uống nước và nghỉ chút!",
        "You worked hard! Take a sip of water and stretch!"
    },
    {
        "ESP32-S3 có cảm biến nhiệt độ bên trong đo liên tục.",
        "ESP32-S3 internal temperature sensor monitors in real-time."
    },
    {
        "Mọi thao tác chạm cảm ứng đều được lọc nhiễu chính xác.",
        "Touch screen is debounced and calibrated accurately."
    },
    {
        "Ami chúc bạn một ngày ngập tràn niềm vui và may mắn!",
        "Ami wishes you an awesome day full of happiness!"
    },
    {
        "Nếu cảm ứng bị lệch, hãy cân chỉnh lại trong Cài đặt.",
        "If touch feels off, re-calibrate anytime in Settings."
    }
};
static const size_t s_adviceCount = sizeof(s_adviceList) / sizeof(s_adviceList[0]);

AppUI::AppUI(DisplayManager& displayManager)
    : display(displayManager) {
    for (uint8_t i = 0; i < WAVE_POINTS; i++) {
        waveBuffer[i] = 1.65f;
    }
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
        case VIEW_GRAPH2:   drawGraph2View(); break;
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
        case VIEW_GRAPH2:
            drawGraph2View();
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
    if (currentView == VIEW_GRAPH1)   title = tr("ĐỒ THỊ 1 (THỜI GIAN THỰC)", "GRAPH 1 (REAL-TIME)");
    if (currentView == VIEW_GRAPH2)   title = tr("ĐỒ THỊ 2 (PHÂN TÍCH ĐA KÊNH)", "GRAPH 2 (TELEMETRY)");
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

    const char* const tabs_vi[] = { "TRANG CHỦ", "ĐỒ THỊ 1", "ĐỒ THỊ 2", "CÀI ĐẶT" };
    const char* const tabs_en[] = { "HOME", "GRAPH 1", "GRAPH 2", "SETTINGS" };
    const char* const* tabs = (currentLang == LANG_VI) ? tabs_vi : tabs_en;
    const uint16_t tabW = 120;

    for (uint8_t i = 0; i < 4; i++) {
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

    // 2. Hai thẻ thông số phần cứng
    // Thẻ 1: CPU & Nhiệt độ
    tft.fillRoundRect(10, 164, 124, 108, 8, CLR_SURFACE);
    tft.drawRoundRect(10, 164, 124, 108, 8, CLR_BORDER);
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tr("CPU & NHIỆT ĐỘ", "CPU & TEMP"), 18, 172);

    // Thẻ 2: Bộ nhớ RAM
    tft.fillRoundRect(144, 164, 124, 108, 8, CLR_SURFACE);
    tft.drawRoundRect(144, 164, 124, 108, 8, CLR_BORDER);
    tft.setTextColor(CLR_SILVER, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tr("BỘ NHỚ RAM", "RAM MEMORY"), 152, 172);

    // Đọc thông số phần cứng thực tế từ ESP32-S3
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

    // 1. Nhiệt độ chip thực tế
    float tempC = temperatureRead();
    if (tempC < 15.0f || tempC > 95.0f) tempC = 37.8f; // Fallback an toàn

    // Thẻ 1: Nhiệt độ CPU
    tft.fillRect(18, 196, 110, 24, CLR_SURFACE);
    tft.setTextColor(tempC > 55.0f ? CLR_AMBER : CLR_CYAN, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f°C", tempC);
    tft.drawString(buf, 18, 196);

    tft.fillRect(18, 246, 110, 20, CLR_SURFACE);
    tft.setTextColor(CLR_GREEN, CLR_SURFACE);
    tft.drawString(tr("[ ỔN ĐỊNH 240M ]", "[ STABLE 240M ]"), 18, 246);

    // 2. RAM thực tế từ ESP32-S3
    uint32_t freeH = ESP.getFreeHeap() / 1024;
    uint32_t totalH = ESP.getHeapSize() / 1024;
    if (totalH == 0) totalH = 320;
    uint32_t usedH = (totalH > freeH) ? (totalH - freeH) : 44;

    tft.fillRect(152, 196, 110, 24, CLR_SURFACE);
    tft.setTextColor(CLR_AMBER, CLR_SURFACE);
    snprintf(buf, sizeof(buf), "%u KB", (unsigned int)usedH);
    tft.drawString(buf, 152, 196);

    tft.fillRect(152, 246, 110, 20, CLR_SURFACE);
    tft.setTextColor(CLR_GREEN, CLR_SURFACE);
    snprintf(buf, sizeof(buf), tr("%uKB trống/320K", "%uKB free/320K"), (unsigned int)freeH);
    tft.drawString(buf, 152, 246);
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
// VIEW 1: ĐỒ THỊ 1 (PLOT GRAPH 1 - REAL-TIME)
// ==========================================
void AppUI::drawGraph1View() {
    TFT_eSPI& tft = display.getTft();

    const int gx = 15;
    const int gy = 42;
    const int gw = 450;
    const int gh = 180;

    tft.fillRect(gx, gy, gw, gh, TFT_BLACK);
    tft.drawRect(gx, gy, gw, gh, CLR_BORDER);

    // Lưới toạ độ
    for (int y = gy + 36; y < gy + gh; y += 36) {
        for (int x = gx + 4; x < gx + gw; x += 10) {
            tft.drawPixel(x, y, CLR_BORDER);
        }
    }
    for (int x = gx + 75; x < gx + gw; x += 75) {
        for (int y = gy + 4; y < gy + gh; y += 8) {
            tft.drawPixel(x, y, CLR_BORDER);
        }
    }

    // Nhãn trục Y
    tft.setTextColor(CLR_SILVER, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("3.3V", gx + 4, gy + 4);
    tft.drawString("2.5V", gx + 4, gy + 45);
    tft.drawString("1.6V", gx + 4, gy + 88);
    tft.drawString("0.8V", gx + 4, gy + 130);
    tft.drawString("0.0V", gx + 4, gy + 168);

    // Nhãn trục X
    tft.setTextDatum(BR_DATUM);
    tft.drawString("-3.0s", gx + 150, gy + gh - 4);
    tft.drawString("-1.5s", gx + 300, gy + gh - 4);
    tft.drawString(tr("HIỆN TẠI", "NOW"), gx + gw - 6, gy + gh - 4);

    // Thanh điều khiển (Y: 230..275)
    tft.fillRoundRect(15, 230, 450, 44, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 230, 450, 44, 6, CLR_BORDER);

    // Nút Tốc độ quét: 1X / 2X / 4X
    tft.fillRoundRect(22, 234, 100, 36, 4, CLR_SURFACE_HI);
    tft.drawRoundRect(22, 234, 100, 36, 4, CLR_BORDER);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    char spdBuf[20];
    snprintf(spdBuf, sizeof(spdBuf), tr("TỐC ĐỘ: %uX", "SPEED: %uX"), waveSpeed);
    tft.drawString(spdBuf, 72, 252);

    // Nút Dạng sóng: Sine / Tim / Tam giác
    tft.fillRoundRect(130, 234, 140, 36, 4, CLR_SURFACE_HI);
    tft.drawRoundRect(130, 234, 140, 36, 4, CLR_CYAN);
    const char* const mNames_vi[] = { "SÓNG: SINE", "SÓNG: TIM", "SÓNG: TAM GIÁC" };
    const char* const mNames_en[] = { "WAVE: SINE", "WAVE: ECG", "WAVE: TRIANGLE" };
    const char* const* mNames = (currentLang == LANG_VI) ? mNames_vi : mNames_en;
    tft.drawString(mNames[waveMode], 200, 252);

    // Nút Tạm dừng / Tiếp tục
    tft.fillRoundRect(280, 234, 95, 36, 4, wavePaused ? CLR_AMBER : CLR_SURFACE_HI);
    tft.drawRoundRect(280, 234, 95, 36, 4, CLR_WHITE);
    tft.setTextColor(wavePaused ? TFT_BLACK : CLR_WHITE, wavePaused ? CLR_AMBER : CLR_SURFACE_HI);
    tft.drawString(wavePaused ? tr("TIẾP TỤC", "RESUME") : tr("TẠM DỪNG", "PAUSE"), 327, 252);

    // Thông số tức thời
    tft.setTextColor(CLR_CYAN, CLR_SURFACE);
    tft.setTextDatum(MR_DATUM);
    tft.drawString("RMS: 2.2V", 455, 252);
}

void AppUI::updateGraph1Wave() {
    if (wavePaused || currentView != VIEW_GRAPH1) return;

    uint32_t now = millis();
    uint32_t interval = (waveSpeed == 4) ? 15 : (waveSpeed == 2 ? 25 : 40);
    if (now - lastWaveTick < interval) return;
    lastWaveTick = now;

    // Sinh điểm sóng
    wavePhase += 0.22f;
    float sample = 1.65f;
    if (waveMode == 0) {
        sample = 1.65f + 1.25f * sinf(wavePhase) + 0.25f * sinf(wavePhase * 2.8f);
    } else if (waveMode == 1) {
        float modP = fmodf(wavePhase, 6.28f);
        if (modP < 0.3f) sample = 1.65f + 1.4f * sinf(modP / 0.3f * 3.14f);
        else if (modP < 0.6f) sample = 1.65f - 0.6f * sinf((modP - 0.3f) / 0.3f * 3.14f);
        else sample = 1.65f + 0.15f * sinf(wavePhase);
    } else {
        // Sóng tam giác
        float modP = fmodf(wavePhase, 3.14f);
        sample = 0.4f + (modP / 3.14f) * 2.5f;
    }

    for (uint8_t i = 0; i < WAVE_POINTS - 1; i++) {
        waveBuffer[i] = waveBuffer[i + 1];
    }
    waveBuffer[WAVE_POINTS - 1] = sample;

    TFT_eSPI& tft = display.getTft();
    const int gx = 55;
    const int gy = 44;
    const int gw = 405;
    const int gh = 175;

    tft.fillRect(gx, gy + 2, gw - 2, gh - 4, TFT_BLACK);

    // Lưới mờ
    for (int y = gy + 35; y < gy + gh - 10; y += 35) {
        for (int x = gx; x < gx + gw; x += 14) {
            tft.drawPixel(x, y, CLR_BORDER);
        }
    }

    // Vẽ nét sóng
    float dx = (float)gw / (WAVE_POINTS - 1);
    int lastX2 = gx, lastY2 = gy;
    for (uint8_t i = 0; i < WAVE_POINTS - 1; i++) {
        int x1 = gx + (int)(i * dx);
        int y1 = gy + gh - (int)((waveBuffer[i] / 3.3f) * gh);
        int x2 = gx + (int)((i + 1) * dx);
        int y2 = gy + gh - (int)((waveBuffer[i + 1] / 3.3f) * gh);

        y1 = constrain(y1, gy + 2, gy + gh - 2);
        y2 = constrain(y2, gy + 2, gy + gh - 2);

        tft.drawLine(x1, y1, x2, y2, CLR_CYAN);
        tft.drawLine(x1, y1 + 1, x2, y2 + 1, CLR_CYAN);
        lastX2 = x2;
        lastY2 = y2;
    }

    // Điểm dạ quang ở đầu mút của sóng
    tft.fillCircle(lastX2, lastY2, 4, CLR_AMBER);
    tft.drawCircle(lastX2, lastY2, 5, CLR_WHITE);
}

// ==========================================
// VIEW 2: ĐỒ THỊ 2 (TELEMETRY BARS & INSPECTOR)
// ==========================================
void AppUI::drawGraph2View() {
    TFT_eSPI& tft = display.getTft();

    const int cx = 15;
    const int cy = 42;
    const int cw = 450;
    const int ch = 180;

    tft.fillRoundRect(cx, cy, cw, ch, 8, CLR_SURFACE);
    tft.drawRoundRect(cx, cy, cw, ch, 8, CLR_BORDER);

    tft.setTextColor(CLR_AMBER, CLR_SURFACE);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tr("6 KÊNH TELEMETRY (CHẠM CỘT ĐỂ XEM CHI TIẾT)", "6 TELEMETRY CHANNELS (TAP BAR FOR DETAILS)"), cx + 12, cy + 8);

    const int baselineY = cy + ch - 26;
    tft.drawFastHLine(cx + 15, baselineY, cw - 30, CLR_BORDER);

    const char* const labels_vi[] = { "Pin", "Nhiệt", "Độ ẩm", "Sáng", "Rung", "RAM" };
    const char* const labels_en[] = { "Bat", "Temp", "Humi", "Light", "Vib", "RAM" };
    const char* const* labels = (currentLang == LANG_VI) ? labels_vi : labels_en;
    const uint16_t colors[] = { CLR_CYAN, CLR_AMBER, CLR_GREEN, CLR_YELLOW, CLR_MAGENTA, CLR_CYAN };
    const int barWidth = 44;
    const int spacing = 68;
    const int startX = cx + 30;

    for (uint8_t i = 0; i < 6; i++) {
        int x = startX + i * spacing;
        int barH = (int)((barValues[i] / 100.0f) * 110);
        int barY = baselineY - barH;

        bool isSel = (selectedBar == i);

        tft.fillRoundRect(x, barY, barWidth, barH, 4, colors[i]);
        tft.drawRoundRect(x, barY, barWidth, barH, 4, isSel ? CLR_AMBER : CLR_WHITE);

        if (isSel) {
            tft.drawRoundRect(x - 2, barY - 2, barWidth + 4, barH + 4, 6, CLR_AMBER);
        }

        tft.setTextColor(isSel ? CLR_AMBER : CLR_WHITE, CLR_SURFACE);
        tft.setTextDatum(BC_DATUM);
        char valStr[12];
        snprintf(valStr, sizeof(valStr), "%d%%", barValues[i]);
        tft.drawString(valStr, x + barWidth / 2, barY - 3);

        tft.setTextColor(isSel ? CLR_AMBER : CLR_SILVER, CLR_SURFACE);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(labels[i], x + barWidth / 2, baselineY + 6);
    }

    // Thanh Inspector hoặc nút điều khiển bên dưới
    drawBarInspector();
}

void AppUI::drawBarInspector() {
    TFT_eSPI& tft = display.getTft();

    tft.fillRoundRect(15, 230, 450, 44, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 230, 450, 44, 6, CLR_BORDER);

    if (selectedBar >= 0 && selectedBar < 6) {
        const char* const fullNames_vi[] = {
            "Dung lượng Pin dự phòng",
            "Nhiệt độ vi xử lý CPU",
            "Độ ẩm môi trường phòng",
            "Cường độ ánh sáng phòng",
            "Mức rung động cảm biến",
            "Dung lượng bộ nhớ RAM"
        };
        const char* const fullNames_en[] = {
            "Backup Battery Level",
            "CPU Core Temperature",
            "Ambient Room Humidity",
            "Ambient Light Intensity",
            "Sensor Vibration Level",
            "RAM Memory Usage"
        };
        const char* const eval_vi[] = {
            "RẤT TỐT", "MÁT MẺ", "LÝ TƯỞNG", "ỔN ĐỊNH", "AN TOÀN", "XUẤT SẮC"
        };
        const char* const eval_en[] = {
            "EXCELLENT", "COOL", "OPTIMAL", "STABLE", "NORMAL", "SUPERB"
        };

        const char* const* fullNames = (currentLang == LANG_VI) ? fullNames_vi : fullNames_en;
        const char* const* eval = (currentLang == LANG_VI) ? eval_vi : eval_en;

        tft.setTextColor(CLR_WHITE, CLR_SURFACE);
        tft.setTextDatum(ML_DATUM);
        char buf[96];
        snprintf(buf, sizeof(buf), "%s: %u%% [%s]", fullNames[selectedBar], barValues[selectedBar], eval[selectedBar]);
        tft.drawString(buf, 25, 252);
    } else {
        // Nút Live stream Toggle
        tft.fillRoundRect(25, 234, 180, 36, 4, liveStreamBars ? CLR_GREEN : CLR_SURFACE_HI);
        tft.drawRoundRect(25, 234, 180, 36, 4, CLR_WHITE);
        tft.setTextColor(liveStreamBars ? TFT_BLACK : CLR_SILVER, liveStreamBars ? CLR_GREEN : CLR_SURFACE_HI);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(liveStreamBars 
            ? tr("TỰ ĐỘNG ĐO: BẬT", "AUTO STREAM: ON") 
            : tr("TỰ ĐỘNG ĐO: TẮT", "AUTO STREAM: OFF"), 115, 252);

        // Nút làm mới tức thì
        tft.fillRoundRect(240, 234, 210, 36, 4, CLR_SURFACE_HI);
        tft.drawRoundRect(240, 234, 210, 36, 4, CLR_CYAN);
        tft.setTextColor(CLR_WHITE, CLR_SURFACE_HI);
        tft.drawString(tr("LÀM MỚI TOÀN BỘ", "REFRESH ALL"), 345, 252);
    }
}

void AppUI::randomizeGraph2Bars() {
    for (uint8_t i = 0; i < 6; i++) {
        int delta = random(-6, 7);
        int nv = barValues[i] + delta;
        barValues[i] = constrain(nv, 25, 98);
    }
    if (currentView == VIEW_GRAPH2) {
        drawGraph2View();
    }
}

// ==========================================
// VIEW 3: CÀI ĐẶT HỆ THỐNG (SETTINGS)
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

    // Hàng 3: Thời gian vuốt ve Ami (y: 116..150)
    tft.fillRoundRect(15, 116, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 116, 450, 34, 6, CLR_BORDER);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(tr("Thời gian vuốt ve Ami:", "Ami Pet Timeout:"), 25, 133);

    const uint8_t petOpts[] = { 4, 6, 10 };
    for (uint8_t i = 0; i < 3; i++) {
        int bx = 240 + i * 68;
        bool isSel = (petTimeoutSec == petOpts[i]);
        tft.fillRoundRect(bx, 119, 60, 28, 4, isSel ? CLR_CYAN : CLR_SURFACE_HI);
        tft.drawRoundRect(bx, 119, 60, 28, 4, isSel ? CLR_WHITE : CLR_BORDER);
        tft.setTextColor(isSel ? TFT_BLACK : CLR_WHITE, isSel ? CLR_CYAN : CLR_SURFACE_HI);
        tft.setTextDatum(MC_DATUM);
        char pBuf[12];
        snprintf(pBuf, sizeof(pBuf), "%us", petOpts[i]);
        tft.drawString(pBuf, bx + 30, 133);
    }

    // Hàng 4: Tự động đổi lời khuyên (y: 154..188)
    tft.fillRoundRect(15, 154, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 154, 450, 34, 6, CLR_BORDER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.drawString(tr("Tự đổi lời khuyên (8s):", "Auto-Cycle Advice (8s):"), 25, 171);

    tft.fillRoundRect(330, 157, 115, 28, 4, autoCycleAdvice ? CLR_GREEN : CLR_SURFACE_HI);
    tft.drawRoundRect(330, 157, 115, 28, 4, CLR_WHITE);
    tft.setTextColor(autoCycleAdvice ? TFT_BLACK : CLR_SILVER, autoCycleAdvice ? CLR_GREEN : CLR_SURFACE_HI);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(autoCycleAdvice ? tr("BẬT (ON)", "ON") : tr("TẮT (OFF)", "OFF"), 387, 171);

    // Hàng 5: Biểu cảm của Ami (y: 192..226)
    tft.fillRoundRect(15, 192, 450, 34, 6, CLR_SURFACE);
    tft.drawRoundRect(15, 192, 450, 34, 6, CLR_BORDER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(CLR_WHITE, CLR_SURFACE);
    tft.drawString(tr("Biểu cảm của Ami:", "Ami's Expression:"), 25, 209);

    const char* const emoNames[] = { "Normal", "Happy", "Idle", "Info" };
    for (uint8_t i = 0; i < 4; i++) {
        int ex = 195 + i * 64;
        tft.fillRoundRect(ex, 195, 58, 28, 4, CLR_SURFACE_HI);
        tft.drawRoundRect(ex, 195, 58, 28, 4, CLR_BORDER);
        tft.setTextColor(CLR_WHITE, CLR_SURFACE_HI);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(emoNames[i], ex + 29, 209);
    }

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

    // 3. Tự động stream dao động nhẹ ở View 2 nếu đang bật Live mode
    if (currentView == VIEW_GRAPH2 && liveStreamBars && selectedBar < 0) {
        if (now - lastBarStreamTick >= 1500) {
            lastBarStreamTick = now;
            randomizeGraph2Bars();
        }
    }

    // 4. Cập nhật thông số phần cứng định kỳ mỗi 2 giây ở View 0
    if (currentView == VIEW_MAIN && (now - lastMetricsUpdate >= 2000)) {
        lastMetricsUpdate = now;
        updateRealMetrics();
    }

    // 5. Cập nhật đồng hồ và header mỗi 1 giây
    if (now - lastHeaderUpdate >= 1000) {
        lastHeaderUpdate = now;
        updateHeaderStats();
    }

    // 6. Xử lý cảm ứng
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

    // A. Chạm vào thanh Navigation Menu ở đáy màn hình (Y: 278..320)
    if (ty >= 278) {
        if (tx < 120) {
            switchView(VIEW_MAIN);
        } else if (tx < 240) {
            switchView(VIEW_GRAPH1);
        } else if (tx < 360) {
            switchView(VIEW_GRAPH2);
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
        // Nút Tốc độ: (22, 234, 100, 36)
        if (tx >= 22 && tx <= 122 && ty >= 230 && ty <= 274) {
            waveSpeed = (waveSpeed == 1) ? 2 : ((waveSpeed == 2) ? 4 : 1);
            drawGraph1View();
            return;
        }
        // Nút Dạng sóng: (130, 234, 140, 36)
        if (tx >= 130 && tx <= 270 && ty >= 230 && ty <= 274) {
            waveMode = (waveMode + 1) % 3;
            drawGraph1View();
            return;
        }
        // Nút Tạm dừng / Tiếp tục: (280, 234, 95, 36)
        if (tx >= 280 && tx <= 375 && ty >= 230 && ty <= 274) {
            wavePaused = !wavePaused;
            drawGraph1View();
            return;
        }
    }
    else if (currentView == VIEW_GRAPH2) {
        // Kiểm tra chạm vào 6 cột để xem Inspector
        const int cx = 15;
        const int cy = 42;
        const int ch = 180;
        const int baselineY = cy + ch - 26;
        const int barWidth = 44;
        const int spacing = 68;
        const int startX = cx + 30;

        bool touchedBar = false;
        for (uint8_t i = 0; i < 6; i++) {
            int bx = startX + i * spacing;
            if (tx >= bx - 5 && tx <= bx + barWidth + 5 && ty >= cy + 20 && ty <= baselineY + 15) {
                selectedBar = (selectedBar == i) ? -1 : i; // Chạm lần 2 thì bỏ chọn
                drawGraph2View();
                touchedBar = true;
                break;
            }
        }
        if (touchedBar) return;

        // Bấm nút bên dưới thanh Inspector
        if (selectedBar < 0) {
            // Nút Toggle Live stream: (25, 234, 180, 36)
            if (tx >= 25 && tx <= 205 && ty >= 230 && ty <= 274) {
                liveStreamBars = !liveStreamBars;
                drawBarInspector();
                return;
            }
            // Nút Làm mới toàn bộ: (240, 234, 210, 36)
            if (tx >= 240 && tx <= 450 && ty >= 230 && ty <= 274) {
                randomizeGraph2Bars();
                return;
            }
        } else {
            // Đang mở chi tiết, chạm thanh bên dưới để đóng lại
            if (ty >= 230 && ty <= 274) {
                selectedBar = -1;
                drawGraph2View();
                return;
            }
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

        // Hàng 3: Chọn thời gian Pet Timeout: 4s, 6s, 10s (y: 116..150)
        if (ty >= 116 && ty <= 150) {
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

        // Hàng 4: Nút Bật/Tắt xoay lời khuyên (y: 154..188)
        if (ty >= 154 && ty <= 188) {
            if (tx >= 320 && tx <= 450) {
                autoCycleAdvice = !autoCycleAdvice;
                drawSettingsView();
                return;
            }
        }

        // Hàng 5: Nút Biểu cảm Ami: Normal, Happy, Idle, Info (y: 192..226)
        if (ty >= 192 && ty <= 226) {
            const char* const emoTags[] = { "normal", "happy", "idle1", "give_info" };
            for (uint8_t i = 0; i < 4; i++) {
                int ex = 195 + i * 64;
                if (tx >= ex && tx <= ex + 58) {
                    if (cornerGif) cornerGif->playByName(emoTags[i]);
                    updateHeaderStats();
                    return;
                }
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
