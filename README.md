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
| **Buttons** | GPIO0 (BOOT) + GPIO14 — brightness + settings |

## Software Stack

| Layer | Technology |
|---|---|
| **Framework** | ESP-IDF v6.0.0 (via PlatformIO) |
| **Graphics** | LVGL 9.5.0 |
| **LVGL Port** | [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port) |
| **UI** | Hand-written LVGL code (no external UI designer) |
| **Provisioning** | espressif/network_provisioning ^1.2.4 (BLE) |
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
│   ├── settings/              # Persistent configuration via NVS
│   │   └── README.md          # 📖 Settings architecture & API docs
│   ├── sntp_sync/             # SNTP time synchronization
│   │   └── README.md          # 📖 SNTP architecture & API docs
│   ├── ui/                    # Hand-written LVGL UI
│   │   ├── README.md          # 📖 Layout, constraints & widget docs
│   │   ├── ui.h               # Public API + widget handles
│   │   ├── ui.c               # Screen creation + widget layout
│   │   └── prov_screen.c/.h   # QR code overlay for BLE provisioning
│   ├── wifi_manager/          # WiFi connection + BLE provisioning fallback
│   │   └── README.md          # 📖 WiFi manager API & architecture
│   └── ble_provisioning/      # BLE provisioning (espressif/network_provisioning)
│       ├── include/ble_provisioning.h
│       ├── src/ble_provisioning.c
│       ├── CMakeLists.txt
│       └── idf_component.yml
├── include/
│   └── board_config.h         # Pin definitions and board constants (single source)
├── src/
│   ├── main.c                 # Application entry point
│   ├── app_handlers.c/.h      # Event callbacks (button, WiFi, BLE, SNTP)
│   ├── CMakeLists.txt         # Main component build config
│   └── idf_component.yml      # LVGL + esp_lvgl_port dependencies
├── platformio.ini
├── partitions.csv
└── sdkconfig.lilygo-t-display-s3
```

> **Component docs:** See [`components/bsp/README.md`](components/bsp/README.md), [`components/ui/README.md`](components/ui/README.md), [`components/sntp_sync/README.md`](components/sntp_sync/README.md), [`components/settings/README.md`](components/settings/README.md), and [`components/wifi_manager/README.md`](components/wifi_manager/README.md) for detailed API documentation.

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

**On first boot (no WiFi credentials in NVS):**
1. Display shows ZenClock UI with smooth 2-second backlight fade-in
2. WiFi manager fires `WIFI_MGR_NO_CRED` event → triggers BLE provisioning
3. BLE provisioning QR code overlay appears on display
4. Use the [Espressif BLE Provisioning app](https://github.com/espressif/esp-idf/tree/master/tools/esp_prov) to scan QR and provision WiFi
5. Once connected, provisioning screen closes and clock displays time (synced via SNTP)

**Button Controls:**
- **BOOT button (GPIO0)**
  - Single-click: Brightness UP (+10%)
  - Double-click (500ms): Toggle Light/Dark theme
- **Side button (GPIO14)**
  - Single-click: Brightness DOWN (-10%)
  - Double-click (500ms): Clear WiFi credentials → restart BLE provisioning

Battery status is displayed in the top-right corner.

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
