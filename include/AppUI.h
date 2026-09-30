#pragma once
#include <Arduino.h>
#include "DisplayManager.h"
#include "CornerGif.h"

enum AppView {
    VIEW_MAIN = 0,
    VIEW_GRAPH1,
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

    // Đồng bộ dữ liệu y sinh & AI thời gian thực
    void pushRealECGSample(float filteredVal);
    void pushRealECGSamples(float rawVal, float filteredVal);
    void setBiometrics(float ecgBpm, float spo2, float ppgBpm, int aiClass, const char* aiClassName);

    void setLanguage(AppLanguage lang);
    void toggleLanguage();
    AppLanguage getLanguage() const { return currentLang; }
    const char* tr(const char* vi, const char* en) const { return (currentLang == LANG_VI) ? vi : en; }

    bool isAiEnabled() const { return aiEnabled; }
    void setAiEnabled(bool en) { aiEnabled = en; }
    void toggleAiEnabled() { aiEnabled = !aiEnabled; }

private:
    DisplayManager& display;
    CornerGif* cornerGif = nullptr;
    AppView currentView = VIEW_MAIN;
    AppLanguage currentLang = LANG_VI;

    // Biến lưu trữ dữ liệu y sinh thực tế
    float realEcgBpm = 0.0f;
    float realSpO2 = 0.0f;
    float realPpgBpm = 0.0f;
    int realAiClass = 0;
    String realAiClassName = "Normal (N)";
    float latestRealECG = 1.65f;
    volatile float latestRealRaw = 2048.0f;
    volatile float latestRealFilt = 0.0f;

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

    // --- View 1: Plot Graph 1 (2 tầng: Raw Đỏ + Filtered Xanh lá giống Python GUI) ---
    void drawGraph1View();
    void updateGraph1Wave();
    static constexpr uint16_t PLOT_X = 45;
    static constexpr uint16_t PLOT_W = 415;
    uint16_t sweepX = 0;
    int16_t lastRawY = -1;
    int16_t lastFiltY = -1;
    uint32_t lastWaveTick = 0;
    uint32_t lastGraphTextUpdate = 0;
    bool wavePaused = false;
    float wavePhase = 0.0f;

    // --- View 2: Cài đặt hệ thống & Tuỳ chọn AI ---
    void drawSettingsView();
    bool aiEnabled = true;
    uint8_t brightnessStep = 4; // 1..5
    uint8_t petTimeoutSec = 6;  // 4s, 6s, 10s
    void applyBrightness(uint8_t step);

    // --- Cảm ứng (Touch Handling) ---
    void handleTouch(uint16_t tx, uint16_t ty);
    uint32_t lastTouchTime = 0;
};
