# ZenClock Project Architecture & Context

**[AI Context & Instructions]**
> This file contains the global architecture map and critical context for the **ZenClock** project. Any AI Agent or LLM assisting with this project MUST read this file to understand the architecture and rules without reading unnecessary source code.

## 1. Project Overview
- **Name**: ZenClock — smart clock on LilyGo T-Display-S3
- **Hardware**: LilyGo T-Display-S3 (ESP32-S3R8, 16MB Flash, 8MB Octal PSRAM)
- **Display**: 1.9" ST7789 320×170 LCD via Intel 8080 8-bit parallel bus
- **Framework**: ESP-IDF v6.0.0 (via PlatformIO)
- **Graphics**: LVGL 9.5.0 + esp_lvgl_port
- **UI**: Hand-written LVGL code (no external UI designer)

## 2. Architecture Map

### `src/main.c`
- Application entry point (`app_main`) only — minimal setup and initialization.
- Calls `bsp_display_init()` → `ui_init()` → `bsp_display_set_brightness()` → `bsp_buttons_init()`.
- All event callbacks (button, WiFi, BLE, SNTP) are in `app_handlers.c`.
- `app_main` returns after setup — FreeRTOS scheduler runs LVGL + event loops.

### `src/app_handlers.c` / `src/app_handlers.h`
- All event/button callbacks: `on_button_press()`, `on_sntp_sync()`, `on_ble_prov_event()`, `on_wifi_event()`.
- Register callbacks at startup: `bsp_buttons_init(on_button_press)`, `wifi_manager_set_callback(on_wifi_event)`, `ble_provisioning_init(on_ble_prov_event)`.
- **Critical**: All LVGL calls must be wrapped in `lvgl_port_lock(0)` / `lvgl_port_unlock()`.

### `components/bsp/` — Board Support Package
Modular HAL for the T-Display-S3 (display, backlight, battery, buttons).
📖 **[Full docs: `bsp/README.md`](components/bsp/README.md)** — architecture, API reference, init sequence, constants.

### `components/ui/` — Hand-Written LVGL UI
Screen creation, theme init, widget layout. No auto-generated code.
📖 **[Full docs: `ui/README.md`](components/ui/README.md)** — layout, constraints, widget handles.

### `include/board_config.h` — Hardware Pin Definitions
**Single source of truth** for all pin assignments and board constants.

### `components/lcd_backlight/`
PWM (LEDC) wrapper for screen brightness (10-bit, 0-100%).
**Do NOT call directly** — use `bsp_display_set_brightness()`.

### `components/sntp_sync/`
SNTP time synchronization (non-blocking task, periodic re-sync).
📖 **[Full docs: `sntp_sync/README.md`](components/sntp_sync/README.md)** — usage and API.

### `components/wifi_manager/`
Single-credential WiFi manager with BLE provisioning fallback. Fires `WIFI_MGR_NO_CRED` event on first boot (no stored credentials) → triggers BLE provisioning.
📖 **[Full docs: `wifi_manager/README.md`](components/wifi_manager/README.md)** — state machine and API.

### `components/ble_provisioning/`
BLE provisioning via espressif/network_provisioning. Security 1 (ECDH + SHA-256), empty PoP, device name `PROV_ZenClock_XXYY`. Shows QR overlay on display. Called on first boot (no credentials) or IO14 double-click. After provisioning succeeds, calls `ble_provisioning_release_memory()` to free BLE RAM (cannot be re-allocated).

### `components/settings/`
Persistent configuration manager via NVS (Non-Volatile Storage).
📖 **[Full docs: `settings/README.md`](components/settings/README.md)** — API and context.

## 3. app_main Pattern

```c
bsp_display_init(&disp_handle, false);
lvgl_port_lock(0);
ui_init();
lvgl_port_unlock();
bsp_display_set_brightness(100, 2000);
bsp_buttons_init(on_button_press);
wifi_manager_set_callback(on_wifi_event);
ble_provisioning_init(on_ble_prov_event);
wifi_manager_start();
// Callbacks in app_handlers.c handle WiFi, BLE, button, SNTP events
```

## 4. Build & Flash

```bash
pio run                  # Build
pio run -t upload        # Flash
pio device monitor       # Serial monitor
pio run -t menuconfig    # SDK configuration
```

## 5. AI Development Rules

1. **Read README first** — `bsp/README.md` and `ui/README.md` before reading `.c` files.
2. **No raw hardware** in `main.c` or UI code — use `bsp` API only.
3. **No direct `lcd_backlight` access** — go through `bsp_display_set_brightness()`.
4. **No `sdkconfig.defaults`** — use `menuconfig`; provide menu paths for config changes.
5. **Never Quad PSRAM** — causes boot loop on this board.
6. **LVGL calls from callbacks** — **ALWAYS** wrap in `lvgl_port_lock(0)` / `lvgl_port_unlock()` when called from WiFi, BLE, button, or SNTP event handlers.
7. **Pin definitions** live in `include/board_config.h` only — never duplicate.
8. **No panel/container objects** — `lv_obj_create(parent)` crashes; place labels directly on screen.
9. **Theme init required** — call `lv_theme_default_init()` before creating widgets.
10. **Re-align after text change** — call `lv_obj_align_to()` when label text changes width.
11. **BLE memory cannot be re-allocated** — After `ble_provisioning_release_memory()` is called, BLE module memory is freed and cannot be re-initialized.

## 6. Pin Reference

| Pin  | Function | Pin  | Function |
| ---- | -------- | ---- | -------- |
| 39–48 | LCD D0–D7 (8-bit data bus) | 8 | WR (PCLK) |
| 6 | CS | 7 | DC |
| 5 | RST | 9 | RD (input, pull-up) |
| 38 | Backlight PWM | 15 | LCD Power (HIGH=on) |
| 4 | Battery ADC (×2 divider) | 0/14 | Buttons (BOOT / IO14) |
