#pragma once
#include <Arduino.h>
#include "DisplayManager.h"
#include "CornerGif.h"

enum AppView {
    VIEW_MAIN = 0,
    VIEW_GRAPH1,
    VIEW_GRAPH2,
    VIEW_SETTINGS
};

enum AppLanguage {
    LANG_VI = 0,
    LANG_EN
};

class AppUI {
public:
    explicit AppUI(DisplayManager& displayManager);

    void init();
    void setCornerGif(CornerGif* gif);
    void update();
    void switchView(AppView view);
    AppView getCurrentView() const { return currentView; }

    void setLanguage(AppLanguage lang);
    void toggleLanguage();
    AppLanguage getLanguage() const { return currentLang; }
    const char* tr(const char* vi, const char* en) const { return (currentLang == LANG_VI) ? vi : en; }

private:
    DisplayManager& display;
    CornerGif* cornerGif = nullptr;
    AppView currentView = VIEW_MAIN;
    AppLanguage currentLang = LANG_VI;

    // --- Navigation & Header ---
    void drawNavBar();
    void drawHeader();
    void updateHeaderStats();
    uint32_t lastHeaderUpdate = 0;

    // --- View 0: Menu Chính & Linh vật lời khuyên ---
    void drawMainView();
    void drawAdviceBubble(const char* customText = nullptr);
    void nextAdvice();
    void updateRealMetrics();
    uint8_t currentAdviceIdx = 0;
    uint32_t lastAdviceCycle = 0;
    uint32_t lastMetricsUpdate = 0;
    bool autoCycleAdvice = true;
    bool showingPetReaction = false;
    uint32_t petReactionStartTime = 0;

    // --- View 1: Plot Graph 1 (Thời gian thực) ---
    void drawGraph1View();
    void updateGraph1Wave();
    static constexpr uint8_t WAVE_POINTS = 70;
    float waveBuffer[WAVE_POINTS];
    uint8_t waveMode = 0; // 0: Sine, 1: ECG (Nhịp tim), 2: Tam giác
    uint8_t waveSpeed = 1; // 1x, 2x, 4x
    bool wavePaused = false;
    uint32_t lastWaveTick = 0;
    float wavePhase = 0.0f;

    // --- View 2: Plot Graph 2 (Đa kênh phân tích) ---
    void drawGraph2View();
    void randomizeGraph2Bars();
    void drawBarInspector();
    uint8_t barValues[6] = { 78, 62, 85, 48, 35, 92 };
    int8_t selectedBar = -1; // Cột đang được chọn xem chi tiết (-1: chưa chọn)
    bool liveStreamBars = true;
    uint32_t lastBarStreamTick = 0;

    // --- View 3: Cài đặt ---
    void drawSettingsView();
    uint8_t brightnessStep = 4; // 1..5
    uint8_t petTimeoutSec = 6;  // 4s, 6s, 10s
    void applyBrightness(uint8_t step);

    // --- Cảm ứng (Touch Handling) ---
    void handleTouch(uint16_t tx, uint16_t ty);
    uint32_t lastTouchTime = 0;
};
