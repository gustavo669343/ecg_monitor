#pragma once
#include <Arduino.h>
#include <AnimatedGIF.h>
#include <TFT_eSPI.h>
#include "DisplayManager.h"
#include "GifRegistry.h"

/**
 * @brief Component that displays an animated GIF at a fixed screen corner/position,
 * with arbitrary integer scale factor (e.g. 6x: 32x32 -> 192x192) and name-based switching.
 */
class CornerGif {
public:
    explicit CornerGif(DisplayManager& displayManager);
    ~CornerGif();

    /**
     * @brief Loads and initializes an animated GIF from raw data with scaling.
     */
    bool load(const uint8_t* pGifData, size_t size, int16_t posX, int16_t posY, uint8_t scale = 6);

    /**
     * @brief Chuyển đổi và phát GIF theo tên định danh (ví dụ: "handwriting", "star").
     * @param name Tên của GIF trong GIF_REGISTRY
     * @return true nếu tìm thấy và nạp thành công
     */
    bool playByName(const char* name);

    /**
     * @brief Phát GIF theo chỉ số trong danh sách GIF_REGISTRY.
     */
    bool playIndex(size_t index);

    /**
     * @brief Chuyển sang ảnh GIF tiếp theo trong danh sách đã đăng ký.
     */
    void next();

    /**
     * @brief Chuyển về ảnh GIF trước đó.
     */
    void prev();

    /**
     * @brief Lấy tên của ảnh GIF hiện đang phát.
     */
    const char* getCurrentName() const;

    /**
     * @brief Kiểm tra toạ độ chạm có nằm trong vùng của Corner GIF hay không (để chạm là đổi ảnh).
     */
    bool contains(uint16_t px, uint16_t py) const {
        if (!isLoaded) return false;
        return (px >= x && px <= (x + getScaledWidth()) && 
                py >= y && py <= (y + getScaledHeight()));
    }

    /**
     * @brief Thiết lập toạ độ hiển thị cố định.
     */
    void setPosition(int16_t posX, int16_t posY) { x = posX; y = posY; }

    /**
     * @brief Cập nhật khung hình tiếp theo non-blockingly trong loop().
     */
    void update();

    /**
     * @brief Vẽ lại frame hiện tại (ví dụ sau khi xoá màn hình).
     */
    void redraw();

    /**
     * @brief Dừng phát và giải phóng bộ đệm Sprite RAM.
     */
    void stop();

    /**
     * @brief Kích hoạt hẹn giờ tự động quay về biểu cảm mặc định (ví dụ sau khi petted).
     * @param durationMs Thời gian chờ tính bằng mili-giây (mặc định 3000ms = 3 giây).
     * @param revertTo Tên biểu cảm sẽ quay về (mặc định: "normal").
     */
    void setTimeout(uint32_t durationMs = 3000, const char* revertTo = "normal");

    /**
     * @brief Làm mới thời gian timeout (khi người dùng tiếp tục chạm/xoa vào nhân vật).
     */
    void refreshTimeout();

    /**
     * @brief Huỷ bỏ timeout.
     */
    void cancelTimeout();

    /**
     * @brief Kiểm tra xem timeout có đang kích hoạt không.
     */
    bool isTimeoutActive() const { return timeoutActive; }

    void setPetTimeoutMs(uint32_t ms) { defaultPetTimeoutMs = ms; }
    uint32_t getPetTimeoutMs() const { return defaultPetTimeoutMs; }

    uint16_t getScaledWidth() const { return origWidth * scaleFactor; }
    uint16_t getScaledHeight() const { return origHeight * scaleFactor; }
    int16_t getX() const { return x; }
    int16_t getY() const { return y; }

    static void GIFDrawCallback(GIFDRAW *pDraw);

private:
    DisplayManager& display;
    AnimatedGIF gif;
    TFT_eSprite sprite;

    bool isLoaded;
    int16_t x, y;
    uint8_t scaleFactor;
    uint16_t origWidth;
    uint16_t origHeight;
    int currentRegistryIndex;

    uint32_t lastFrameTime;
    int frameDelayMs;

    // Quản lý timeout tự động (ví dụ: petted sau 3 giây quay về normal)
    bool timeoutActive;
    uint32_t timeoutStart;
    uint32_t timeoutDuration;
    uint32_t defaultPetTimeoutMs = 6000;
    char timeoutTarget[32];

    void decodeNextFrame();
    void eraseArea();
};
