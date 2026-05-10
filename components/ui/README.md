# UI — Modular LVGL Interface

> **[AI Context]** This component provides the ZenClock user interface using **hand-written LVGL 9.x code**.
> The UI is split into self-contained widget modules — each module owns its LVGL objects, timers, and update logic.

## Files

| File | Purpose |
|---|---|
| `ui.h` | Public API — `ui_init()` only, no widget handles exposed |
| `ui.c` | Screen + theme + module composition (calls sub-modules) |
| `clock_face.h` | Clock face API — `clock_face_create(parent)` |
| `clock_face_text.c` | Text-based clock implementation (Montserrat 48 time, Montserrat 14 date) |
| `status_bar.h` | Status bar API — `status_bar_create(parent)` |
| `status_bar.c` | Battery icon + percentage + 30s LVGL timer |
| `CMakeLists.txt` | ESP-IDF component registration |

## Architecture

```
main.c ──► ui.h ──► ui_init()
                      ├── clock_face.h ──► clock_face_create(parent)
                      └── status_bar.h ──► status_bar_create(parent)
```

- `main.c` only includes `ui.h` — zero knowledge of widgets or timers
- Each widget module owns its LVGL objects, timers, and update logic
- Swapping clock implementation = replace one `.c` file in CMakeLists

## Public API

Header: [`ui.h`](ui.h)

### `ui_init()`

Creates the screen, initializes the LVGL default theme, and delegates widget creation to sub-modules. Each module creates its own LVGL timer for automatic updates.

**Must be called inside `lvgl_port_lock(0)` / `lvgl_port_unlock()`.**

## Widget Modules

### Clock Face (`clock_face.h`)

- **`clock_face_create(parent)`** — creates time (HH:MM:SS) and date (DD/MM/YYYY) labels
- Internal 1-second LVGL timer for automatic updates
- Private state: `static lv_obj_t *s_time_label`, `static lv_obj_t *s_date_label`

### Status Bar (`status_bar.h`)

- **`status_bar_create(parent)`** — creates battery icon + percentage label
- Internal 30-second LVGL timer for automatic battery polling
- Calls `bsp_battery_get_percentage()` / `bsp_battery_usb_connected()` directly
- Private state: `static lv_obj_t *s_bat_icon`, `static lv_obj_t *s_bat_pct`

## Layout

```
┌─────────────────────────────────────────────────┐
│                                      ⚡ 100%   │  ← status_bar (top-right)
│                                                 │
│                 18:09:30                        │  ← clock_face (Montserrat 48, center)
│                10/05/2026                       │  ← clock_face (Montserrat 14, muted grey)
│                                                 │
│                            FPS: 30 CPU: 2% ... │  ← perf monitor (LVGL overlay)
└─────────────────────────────────────────────────┘
  320 × 170 pixels (landscape)
```

## Swapping Clock Face

```cmake
# Text clock (current)
SRCS "ui.c" "clock_face_text.c" "status_bar.c"

# Flip clock (future — same clock_face.h API)
SRCS "ui.c" "clock_face_flip.c" "status_bar.c"
```

Only 1 line changes. `ui.c`, `main.c`, `status_bar.c` remain untouched.

## Design Constraints

- **No container/panel objects** — `lv_obj_create(parent)` as a container causes a crash (`StoreProhibited` at address -320) during font rendering on this board. Place all labels directly on the screen.
- **Font symbols work fine** — `LV_SYMBOL_BATTERY_FULL`, `LV_SYMBOL_CHARGE` etc. are available in the default `montserrat_14` font.
- **Montserrat 48 required** — The clock time display uses `lv_font_montserrat_48`. Must be enabled via menuconfig: `Component config → LVGL → Font usage → Enable Montserrat 48`.
- **Full-screen LVGL buffer required** — `LVGL_BUF_SIZE = LCD_H_RES * LCD_V_RES` in `bsp_display.c`. Smaller buffers cause rendering artifacts when labels change text.

## Adding New Widget Modules

1. Create `widget.h` with a `widget_create(lv_obj_t *parent)` function
2. Implement in `widget.c` — keep all LVGL objects as `static` locals
3. For periodic updates, create an LVGL timer inside the `_create()` function
4. Call `widget_create(screen)` from `ui_init()` in `ui.c`
5. Add the `.c` file to `SRCS` in `CMakeLists.txt`

## Rules for AI Agents

1. **Never use `lv_obj_create(parent)`** as a panel/container — causes crash on this board.
2. **Use LVGL timers for periodic UI updates** — `lv_timer_create(cb, period_ms, NULL)`. Never use FreeRTOS tasks with `lvgl_port_lock/unlock` for UI updates — causes rendering artifacts on I80 bus.
3. **Call `lv_obj_align_to()` after text changes** if using relative alignment.
4. **Theme init is mandatory** — always call `lv_theme_default_init()` before creating widgets.
5. **Use fixed width for frequently-updated labels** — `LV_SIZE_CONTENT` with large fonts causes smearing when text width changes.
6. **Keep widget handles private** — use `static` in each module, never expose via headers.
7. **Each module owns its timers** — create timers inside `_create()`, not in `main.c`.
