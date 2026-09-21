# Product

## Register

product

## Users
- Embedded developers, IoT enthusiasts, and smart-desk companion users interacting with an ESP32-S3 touchscreen device (ILI9488 480x320).
- Users looking for real-time telemetry, clear signal monitoring, interactive virtual companion feedback, and quick device control.

## Product Purpose
- Provide an intuitive, low-latency, zero-flicker touchscreen dashboard on ESP32-S3 featuring 4 distinct views:
  1. **Menu Chính (Dashboard / Companion)**: Mascot advice speech bubble, quick telemetry cards, interactive petting.
  2. **Đồ thị 1 (Real-time Waveform)**: High-speed continuous sensor/signal plotting with telemetry min/max/avg.
  3. **Đồ thị 2 (Comparative Analytics)**: Multi-channel telemetry or comparative bar chart.
  4. **Cài đặt (Settings)**: Device controls (brightness, advice cycle, calibration, mascot expression).
- Make embedded hardware feel responsive, delightful, and human with an animated companion giving timely advice.

## Brand Personality
- **Voice & Tone**: Friendly, precise, attentive, modern sci-fi desk assistant.
- **3-word personality**: Responsive, Delightful, Purposeful.
- **Emotional Goals**: Calm clarity in telemetry, joy and companionship through mascot reactions.

## Anti-references
- Cluttered 90s PLC industrial screens with tiny illegible buttons.
- Sluggish web-view lag or flickering full-screen refreshes.
- Boring monochromatic flat grey dashboards devoid of warmth or personality.

## Design Principles
1. **Zero Flicker, Direct DMA**: Use off-screen buffering and dirty-region clipping to ensure every transition is fluid without tearing.
2. **Finger-First Ergonomics**: All interactive touch buttons must be at least 44x40 px to ensure 100% first-try hit rate on resistive/capacitive glass.
3. **Companion with Purpose**: The mascot is not decorative clutter; it reacts to states, provides speech advice, and brings the hardware to life.
4. **Data at a Glance**: Telemetry graphs must prioritize signal clarity over decoration, using distinct high-contrast color channels.

## Accessibility & Inclusion
- High contrast (> 4.5:1) for all labels against dark backgrounds (Graphite/Navy with Cyan/Amber/White).
- Visual feedback on every touch (button depression highlight / sound or border feedback).
- Distinct color coding with text labels to support color vision deficiencies.
