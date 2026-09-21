# Design System (ESP32-S3 480x320 TFT)

## Display Specifications
- **Resolution**: 480 x 320 Landscape (ILI9488 SPI driver, 16-bit RGB565)
- **Touch**: XPT2046 SPI Touch Controller with software debounce
- **Refresh Strategy**: Targeted dirty rect redraws + TFT_eSprite buffers for graphs & mascot animations.

## Color Tokens (16-bit RGB565)

| Token Name | RGB565 Hex | Visual Role | Contrast Ratio |
| :--- | :--- | :--- | :--- |
| `COLOR_BG` | `0x0821` (Dark Graphite) | Primary screen canvas background | Anchor |
| `COLOR_SURFACE` | `0x18E3` (Dark Navy/Slate) | Card backgrounds, dialogs, menu bar | 12:1 against White |
| `COLOR_SURFACE_BORDER`| `0x39E7` (Border Slate) | Card borders, dividers | Subdued |
| `COLOR_ACCENT_PRIMARY` | `0x07FF` (Vibrant Cyan) | Active tab highlight, graph trace 1 | 9.5:1 against Slate |
| `COLOR_ACCENT_SECONDARY`| `0xFDE0` (Bright Amber) | Warning, graph trace 2, heart/spark | 8.2:1 against Slate |
| `COLOR_SUCCESS` | `0x07E0` (Neon Emerald) | Normal status, good telemetry | 11:1 against Slate |
| `COLOR_TEXT_PRIMARY` | `0xFFFF` (Pure White) | Primary labels, advice text | 15:1 against BG |
| `COLOR_TEXT_MUTED` | `0x9CD3` (Cool Silver) | Subtitles, units, inactive tabs | 4.8:1 against BG |
| `COLOR_SPEECH_BG` | `0x1948` (Deep Blue Slate)| Mascot advice speech bubble bg | High readability |

## Spatial Architecture (480 x 320)

```
+-------------------------------------------------------------+
| TOP STATUS BAR (0, 0, 480, 36)                              |
| [Status Dot] Title / Tab Name            Uptime: 00:12:45   |
+-------------------------------------------------------------+
| MAIN CONTENT VIEWPORT (0, 36, 480, 244)                     |
|                                                             |
|   Tab 0: [Advice Box + Cards]      [Mascot Companion (192)] |
|   Tab 1: [Real-time Waveform Plot + Stats]                  |
|   Tab 2: [Multi-channel Comparative Plot / Bar Chart]       |
|   Tab 3: [Settings Controls: Brightness, Calib, Options]    |
|                                                             |
+-------------------------------------------------------------+
| BOTTOM NAVIGATION BAR (0, 280, 480, 40)                     |
| [ TRANG CHỦ ]   [ ĐỒ THỊ 1 ]   [ ĐỒ THỊ 2 ]   [ CÀI ĐẶT ]   |
+-------------------------------------------------------------+
```

## Navigation Bar Specification
- **Height**: 40px (`Y: 280..320`)
- **Tabs**: 4 tabs, each width = 120px (`X: 0..119`, `120..239`, `240..359`, `360..479`).
- **Touch Target**: Full $120 \times 40$ px area for effortless thumb/finger actuation.
- **Active State**: Glowing 3px top indicator line in `COLOR_ACCENT_PRIMARY`, highlighted tab label, bright icon/text.

## Mascot Companion & Advice System
- **Mascot Position**: Fixed at `(280, 42)`, size `192 x 192` (2x scaling of 96x96 sprite).
- **Speech Bubble (Khung Lời Khuyên)**: Positioned at `(10, 42, 262, 114)`.
  - Header: `"✦ LỜI KHUYÊN"` in Bright Amber (`0xFDE0`).
  - Text: Multi-line wrapped advice string (up to 3 lines) in Pure White.
  - Tail: Speech bubble tail pointing towards the mascot at `(268, 90)`.
  - Cycling: Tap on the speech bubble or mascot to cycle through advice and trigger character reactions (`speak` / `happy` / `petted`).
