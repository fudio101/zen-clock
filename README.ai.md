# ZenClock Project Architecture & Context

**[AI Context & Instructions]**
> This file contains the global architecture map and critical context for the **ZenClock** project. Any AI Agent or LLM assisting with this project MUST read this file to understand the architecture and rules without reading unnecessary source code. Keep context tight!

## 1. Project Overview
- **Name**: ZenClock — smart clock on LilyGo T-Display-S3
- **Hardware**: LilyGo T-Display-S3 (ESP32-S3R8, 16MB Flash, 8MB Octal PSRAM)
- **Display**: 1.9" ST7789 320×170 LCD via Intel 8080 8-bit parallel bus
- **Framework**: ESP-IDF v6.0.0 (via PlatformIO)
- **Graphics**: LVGL 9.x + esp_lvgl_port
- **UI Design**: SquareLine Studio v2.x (exports C code)

## 2. Architecture Map

### `src/main.c`
- Application entry point (`app_main`).
- Calls `lcd_init()` → `ui_init()` → `lcd_set_brightness()`.
- All LVGL calls wrapped in `lvgl_port_lock(0)` / `lvgl_port_unlock()`.

### `components/tdisplays3/`
- **HAL layer** — hardware abstraction for the T-Display-S3 board.
- Initializes I80 bus, ST7789 panel, LVGL port, backlight, and battery ADC.
- Key APIs:
  ```c
  void lcd_init(lv_disp_t **disp, bool backlight_on);
  void lcd_set_brightness(uint8_t percent, uint32_t fade_ms);
  uint8_t lcd_get_brightness(void);
  int get_battery_voltage(void);       // millivolts (×2 corrected)
  int get_battery_percentage(void);    // 0-100
  bool usb_power_connected(void);
  ```
- 📖 **[Read header: `t_display_s3.h`](components/tdisplays3/t_display_s3.h)** for full API.

### `components/lcd_backlight/`
- High-performance PWM (LEDC) wrapper for screen brightness.
- Uses 10-bit resolution for premium fading effects (0-100%).
- **Do NOT call directly** — use `lcd_set_brightness()` from tdisplays3.
- 📖 **[Read docs: `README.md`](components/lcd_backlight/README.md)**

### `components/ui/` ⚠️ AUTO-GENERATED
- SquareLine Studio export output. **Never edit manually.**
- Contains screens, widgets, fonts, and event hooks.
- After each SquareLine export, run `patch_ui.bat` to fix CMakeLists.txt.
- Access exported widgets via `ui.h` (e.g., `ui_Label1`, `ui_Screen1`).

### `SquareLine/boards/t_display_s3/`
- Board template (`.slb`) for SquareLine Studio.
- Configures 320×170, 16-bit swapped color depth, LVGL 9.x.

## 3. Display Orientation

Rotation MUST be configured in **two places** with matching values:

```c
// Panel hardware (init_lcd_panel):
esp_lcd_panel_swap_xy(panel_handle, true);
esp_lcd_panel_mirror(panel_handle, false, true);
esp_lcd_panel_set_gap(panel_handle, 0, 35);

// LVGL port (lvgl_port_display_cfg_t):
.rotation = { .swap_xy = true, .mirror_x = false, .mirror_y = true }
```

> If these don't match, the display will appear garbled/smeared.

## 4. app_main Pattern

```c
static lv_display_t *disp_handle;
lcd_init(&disp_handle, false);   // backlight off initially
lvgl_port_lock(0);               // MUST lock before any LVGL call
ui_init();                       // SquareLine-generated init
lvgl_port_unlock();
lcd_set_brightness(100, 2000);   // smooth fade-in
```

## 5. Build & Flash Workflow

```bash
pio run                  # Build
pio run -t upload        # Flash
pio device monitor       # Serial monitor
pio run -t menuconfig    # SDK configuration
```

### After SquareLine UI Export:
```bash
patch_ui.bat             # Fix CMakeLists.txt (REQUIRED)
pio run -t upload        # Build & flash
```

## 6. Strict AI Development Rules

1. **No raw hardware** in `main.c` or UI code — use `tdisplays3` API only.
2. **No direct `lcd_backlight` access** — go through `tdisplays3` wrapper.
3. **No `sdkconfig.defaults`** — use `menuconfig`; provide menu paths for config changes.
4. **Never Quad PSRAM** — causes boot loop on this board.
5. **UI is auto-gen** — never edit `components/ui/`; always run `patch_ui.bat` after export.
6. **LVGL calls** must be wrapped in `lvgl_port_lock(0)` / `lvgl_port_unlock()`.
7. Prefer `.h` files and `README.md` for API understanding before reading `.c` files.

## 7. Pin Reference

| Pin  | Function | Pin  | Function |
| ---- | -------- | ---- | -------- |
| 39–48 | LCD D0–D7 (8-bit data bus) | 8 | WR (PCLK) |
| 6 | CS | 7 | DC |
| 5 | RST | 9 | RD (input, pull-up) |
| 38 | Backlight PWM | 15 | LCD Power (HIGH=on) |
| 4 | Battery ADC (×2 divider) | 0/14 | Buttons (BOOT / IO14) |
