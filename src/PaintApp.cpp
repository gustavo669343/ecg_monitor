#include "PaintApp.h"
#include "CornerGif.h"

PaintApp::PaintApp(DisplayManager& displayManager)
    : display(displayManager),
      currentColor(TFT_GREEN),
      brushRadius(4) {
    // Initialize color palette buttons (Bottom row)
    colorBtns[0] = { 10,  280, 50, 35, TFT_RED,     "RED" };
    colorBtns[1] = { 70,  280, 50, 35, TFT_GREEN,   "GRN" };
    colorBtns[2] = { 130, 280, 50, 35, TFT_BLUE,    "BLU" };
    colorBtns[3] = { 190, 280, 50, 35, TFT_YELLOW,  "YEL" };
    colorBtns[4] = { 250, 280, 50, 35, TFT_WHITE,   "WHT" };

    // Action buttons
    btnClear = { 320, 280, 70, 35, TFT_DARKGREY, "CLEAR" };
    btnCalib = { 400, 280, 70, 35, TFT_NAVY,     "CALIB" };
}

void PaintApp::init() {
    drawUI();
}

void PaintApp::clearCanvas() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(0, 35, SCREEN_WIDTH, 240, TFT_BLACK);
    if (cornerGif) {
        cornerGif->redraw();
    }
}


void PaintApp::drawUI() {
    TFT_eSPI& tft = display.getTft();
    tft.fillScreen(TFT_BLACK);

    // Header Bar
    tft.fillRect(0, 0, SCREEN_WIDTH, 35, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.setTextFont(2);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("ESP32-S3 Touch Paint", 10, 10);

    // Color indicator label
    tft.drawString("Color:", 240, 10);
    updateColorIndicator();

    // Canvas border
    tft.drawRect(0, 35, SCREEN_WIDTH, 242, TFT_DARKGREY);

    // Draw color buttons
    for (uint8_t i = 0; i < NUM_COLOR_BTNS; i++) {
        tft.fillRoundRect(colorBtns[i].x, colorBtns[i].y, colorBtns[i].w, colorBtns[i].h, 4, colorBtns[i].color);
        tft.drawRoundRect(colorBtns[i].x, colorBtns[i].y, colorBtns[i].w, colorBtns[i].h, 4, TFT_WHITE);
        tft.setTextColor(colorBtns[i].color == TFT_WHITE || colorBtns[i].color == TFT_YELLOW ? TFT_BLACK : TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(colorBtns[i].label, colorBtns[i].x + colorBtns[i].w / 2, colorBtns[i].y + colorBtns[i].h / 2);
    }

    // Draw CLEAR button
    tft.fillRoundRect(btnClear.x, btnClear.y, btnClear.w, btnClear.h, 4, btnClear.color);
    tft.drawRoundRect(btnClear.x, btnClear.y, btnClear.w, btnClear.h, 4, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(btnClear.label, btnClear.x + btnClear.w / 2, btnClear.y + btnClear.h / 2);

    // Draw CALIB button
    tft.fillRoundRect(btnCalib.x, btnCalib.y, btnCalib.w, btnCalib.h, 4, btnCalib.color);
    tft.drawRoundRect(btnCalib.x, btnCalib.y, btnCalib.w, btnCalib.h, 4, TFT_WHITE);
    tft.drawString(btnCalib.label, btnCalib.x + btnCalib.w / 2, btnCalib.y + btnCalib.h / 2);
}

void PaintApp::updateColorIndicator() {
    TFT_eSPI& tft = display.getTft();
    tft.fillCircle(290, 18, 8, currentColor);
    tft.drawCircle(290, 18, 8, TFT_WHITE);
}

void PaintApp::updateCoordinates(uint16_t x, uint16_t y) {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(310, 5, 165, 25, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setTextFont(2);
    tft.setTextDatum(TL_DATUM);
    char buf[32];
    if (cornerGif) {
        snprintf(buf, sizeof(buf), "[%s] %u,%u", cornerGif->getCurrentName(), x, y);
    } else {
        snprintf(buf, sizeof(buf), "X:%3u  Y:%3u", x, y);
    }
    tft.drawString(buf, 315, 10);
}

void PaintApp::updateHeaderName() {
    TFT_eSPI& tft = display.getTft();
    tft.fillRect(310, 5, 165, 25, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setTextFont(2);
    tft.setTextDatum(TL_DATUM);
    char buf[32];
    if (cornerGif) {
        snprintf(buf, sizeof(buf), "[%s]", cornerGif->getCurrentName());
    } else {
        snprintf(buf, sizeof(buf), "Paint UI");
    }
    tft.drawString(buf, 315, 10);
}

void PaintApp::update() {
    // 0. Tự động đồng bộ tên biểu cảm trên thanh tiêu đề khi trạng thái đổi (ví dụ petted timeout về normal)
    if (cornerGif) {
        const char* curName = cornerGif->getCurrentName();
        if (strcmp(curName, lastGifName) != 0) {
            strncpy(lastGifName, curName, sizeof(lastGifName) - 1);
            updateHeaderName();
        }
    }

    uint16_t x = 0, y = 0;
    if (!display.getTouch(&x, &y)) {
        // Nhấc tay / nhấc bút -> ngắt nét vẽ hiện tại
        lastX = -1;
        lastY = -1;
        return;
    }

    // Update real-time coordinates on top bar
    updateCoordinates(x, y);

    // 1. Check CLEAR button (padding 6px để ngón tay dễ bấm trúng)
    if (btnClear.contains(x, y, 6)) {
        clearCanvas();
        lastX = -1;
        lastY = -1;
        delay(150); // Simple debounce
        return;
    }

    // 2. Check CALIB button
    if (btnCalib.contains(x, y, 6)) {
        display.runCalibration();
        drawUI();
        if (cornerGif) cornerGif->redraw();
        lastX = -1;
        lastY = -1;
        return;
    }

    // 3. Check color selection buttons
    for (uint8_t i = 0; i < NUM_COLOR_BTNS; i++) {
        if (colorBtns[i].contains(x, y, 6)) {
            currentColor = colorBtns[i].color;
            updateColorIndicator();
            lastX = -1;
            lastY = -1;
            delay(100);
            return;
        }
    }

    // 3.5. Chạm / vuốt vào nhân vật góc: Biểu cảm 'petted' (tự động timeout sau 3 giây)
    if (cornerGif && cornerGif->contains(x, y)) {
        if (strcasecmp(cornerGif->getCurrentName(), "petted") == 0) {
            // Đang được xoa đầu mà tiếp tục chạm -> làm mới lại đồng hồ 3 giây
            cornerGif->refreshTimeout();
        } else {
            // Chạm vào nhân vật -> biểu cảm petted xoa đầu trong 3 giây
            cornerGif->playByName("petted");
        }
        updateCoordinates(x, y);
        lastX = -1;
        lastY = -1;
        delay(120); // Debounce ngắn để trải nghiệm xoa đầu liên tục mượt mà
        return;
    }

    // 3.6. Chạm vào thanh tiêu đề góc trên bên phải để chuyển sang GIF kế tiếp (next)
    if (y < 35 && x > 280) {
        if (cornerGif) {
            cornerGif->next();
            updateCoordinates(x, y);
        }
        lastX = -1;
        lastY = -1;
        delay(250);
        return;
    }

    // 4. If touched inside canvas area (between header and bottom buttons)
    if (y > 38 && y < 275) {
        TFT_eSPI& tft = display.getTft();
        if (lastX >= 0 && lastY >= 0 && abs(x - lastX) < 60 && abs(y - lastY) < 60) {
            // Nối nét vẽ liền mạch mượt mà khi di chuyển ngón tay hoặc bút
            tft.drawLine(lastX, lastY, x, y, currentColor);
            tft.drawLine(lastX, lastY - 1, x, y - 1, currentColor);
            tft.drawLine(lastX, lastY + 1, x, y + 1, currentColor);
            tft.fillCircle(x, y, brushRadius, currentColor);
        } else {
            tft.fillCircle(x, y, brushRadius, currentColor);
        }
        lastX = x;
        lastY = y;
    } else {
        lastX = -1;
        lastY = -1;
    }
}


