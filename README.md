# ZenClock

A beautiful clock project running on the **LilyGo T-Display-S3** board.

## Hardware

| Spec | Value |
|---|---|
| **Board** | LilyGo T-Display-S3 (non-touch, v1.2) |
| **MCU** | ESP32-S3R8 (Dual-core Xtensa LX7, 240 MHz) |
| **Flash** | 16 MB |
| **PSRAM** | 8 MB Octal (OPI) |
| **Display** | 1.9" ST7789, 320×170, Intel 8080 8-bit parallel |
| **Backlight** | PWM controlled via ESP-IDF LEDC (lcd_backlight component) |
| **Battery** | GPIO4 ADC via resistor divider |
| **Buttons** | GPIO0 (BOOT) + GPIO14 — brightness control |

## Software Stack

| Layer | Technology |
|---|---|
| **Framework** | ESP-IDF v6.0.0 (via PlatformIO) |
| **Graphics** | LVGL 9.5.0 |
| **LVGL Port** | [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port) |
| **UI** | Hand-written LVGL code (no external UI designer) |
| **Build System** | PlatformIO + ESP-IDF Component Manager |

## Project Structure

```
ZenClock/
├── components/
│   ├── bsp/                   # Board Support Package (modular HAL)
│   │   ├── README.md          # 📖 Detailed architecture & API docs
│   │   ├── include/bsp.h      # Public API
│   │   ├── priv_include/      # Internal cross-module declarations
│   │   └── src/
│   │       ├── bsp_display.c  # I80 bus + ST7789 + LVGL port
│   │       ├── bsp_battery.c  # ADC + voltage + percentage
│   │       ├── bsp_backlight.c # Brightness facade
│   │       └── bsp_buttons.c  # Button ISR + debounce
│   ├── lcd_backlight/         # PWM backlight driver (LEDC wrapper)
│   └── ui/                    # Hand-written LVGL UI
│       ├── README.md          # 📖 Layout, constraints & widget docs
│       ├── ui.h               # Public API + widget handles
│       └── ui.c               # Screen creation + widget layout
├── include/
│   └── board_config.h         # Pin definitions and board constants (single source)
├── src/
│   ├── main.c                 # Application entry point + battery task
│   ├── CMakeLists.txt         # Main component build config
│   └── idf_component.yml      # LVGL + esp_lvgl_port dependencies
├── platformio.ini
├── partitions.csv
└── sdkconfig.lilygo-t-display-s3
```

> **Component docs:** See [`components/bsp/README.md`](components/bsp/README.md) and [`components/ui/README.md`](components/ui/README.md) for detailed API documentation.

## Getting Started

### Prerequisites

1. [PlatformIO](https://platformio.org/) installed (VS Code extension recommended)
2. LilyGo T-Display-S3 connected via USB

### Build & Flash

```bash
# Build the project
pio run

# Flash to device
pio run -t upload

# Monitor serial output
pio device monitor
```

### First Boot

After flashing, the display shows the ZenClock UI with a smooth 2-second backlight fade-in. Battery status is displayed in the top-right corner. Use the buttons to adjust brightness:
- **BOOT button** (GPIO0) — Brightness UP (+10%)
- **Side button** (GPIO14) — Brightness DOWN (-10%)

---

## Menuconfig (Required Settings)

After cloning or setting up the project for the first time, run:

```bash
pio run -t menuconfig
```

Ensure the following settings are configured:

| Setting | Path | Value |
|---|---|---|
| PSRAM Mode | Component config → ESP PSRAM → SPI RAM config → Mode | **Octal** |
| PSRAM Speed | Component config → ESP PSRAM → SPI RAM config → Speed | **80 MHz** |
| CPU Frequency | Component config → ESP System Settings → CPU frequency | **240 MHz** |
| FreeRTOS Tick | Component config → FreeRTOS → Kernel → Tick rate (Hz) | **1000** |
| Flash Size | Serial flasher config → Flash size | **16 MB** |
| LVGL Color Depth | Component config → LVGL → Display → Color depth | **16** |
| Montserrat 14 | Component config → LVGL → Font usage → Enable Montserrat 14 | **✓** |

> ⚠️ **Never set PSRAM to Quad mode** — this will cause a boot loop on the T-Display-S3.

---

## Known Gotchas & Troubleshooting

### Display Garbled / Smeared

The ST7789 display orientation must be configured in **two places** with matching values. See [`components/bsp/README.md`](components/bsp/README.md#display-orientation) for details.

### Rendering Artifacts

LVGL buffer must be set to **full screen** (`LCD_H_RES * LCD_V_RES`) in `bsp_display.c`. Smaller buffers cause smearing when labels update text dynamically.

### Panel Container Crash

Do **not** use `lv_obj_create(parent)` as a container/panel in the UI — it causes a `StoreProhibited` crash during font rendering. Place all widgets directly on the screen object. See [`components/ui/README.md`](components/ui/README.md#design-constraints) for details.

---

## Credits

- Display driver architecture based on [hiruna/esp-idf-t-display-s3](https://github.com/hiruna/esp-idf-t-display-s3)
- Backlight driver adapted from [hiruna/esp-idf-aw9364](https://github.com/hiruna/esp-idf-aw9364)
- [LVGL](https://lvgl.io/) — Light and Versatile Graphics Library
- [Espressif esp_lvgl_port](https://github.com/espressif/esp-bsp)

## License

MIT
