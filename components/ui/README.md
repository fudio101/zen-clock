# UI — Hand-Written LVGL Interface

> **[AI Context]** This component provides the ZenClock user interface using **hand-written LVGL 9.x code**.
> SquareLine Studio is **no longer used** — all UI is defined in `ui.c`.

## Files

| File | Purpose |
|---|---|
| `ui.h` | Public API + exported widget handles |
| `ui.c` | Screen creation, theme init, widget layout |
| `CMakeLists.txt` | ESP-IDF component registration |

## Public API

Header: [`ui.h`](ui.h)

### `ui_init()`

Creates all screens and widgets, initializes the LVGL default theme, and loads the main screen.

**Must be called inside `lvgl_port_lock(0)` / `lvgl_port_unlock()`.**

### Exported Widget Handles

These globals are set by `ui_init()` and can be updated from `main.c` (always inside `lvgl_port_lock/unlock`):

| Handle | Type | Purpose |
|---|---|---|
| `ui_screen_main` | Screen | Main (and only) screen |
| `ui_bat_icon_label` | Label | Battery icon — uses `LV_SYMBOL_BATTERY_*` or `LV_SYMBOL_CHARGE` |
| `ui_bat_pct_label` | Label | Battery percentage text (e.g. `"85%"`, `"N/A"`) |
| `ui_clock_label` | Label | Center content area (placeholder for clock) |

## Layout

```
┌─────────────────────────────────────────────────┐
│                          ⚡ 100% │  ← top-right
│                                                 │
│              ZenClock Ready                     │  ← center
│                                                 │
│                            FPS: 30 CPU: 2% ... │  ← perf monitor (LVGL overlay)
└─────────────────────────────────────────────────┘
  320 × 170 pixels (landscape)
```

- Battery icon uses `lv_obj_align_to()` relative to `ui_bat_pct_label` — auto-adjusts when text width changes.
- After updating label text, call `lv_obj_align_to()` to re-position the icon.

## Theme

`ui_init()` calls `lv_theme_default_init()` with `dark=false` (LVGL default light theme). This **must** be called before creating widgets — otherwise LVGL objects may lack proper default styles.

## Design Constraints

- **No container/panel objects** — `lv_obj_create(parent)` as a container causes a crash (`StoreProhibited` at address -320) during font rendering on this board. Place all labels directly on the screen.
- **Font symbols work fine** — `LV_SYMBOL_BATTERY_FULL`, `LV_SYMBOL_CHARGE` etc. are available in the default `montserrat_14` font.
- **Full-screen LVGL buffer required** — `LVGL_BUF_SIZE = LCD_H_RES * LCD_V_RES` in `bsp_display.c`. Smaller buffers cause rendering artifacts when labels change text.

## Adding New Widgets

1. Add the `lv_obj_t *` global in `ui.c` and `extern` in `ui.h`
2. Create the widget inside `ui_init()` on `ui_screen_main`
3. Use `LV_SIZE_CONTENT` for auto-sized labels
4. Use `lv_obj_align()` or `lv_obj_align_to()` for positioning
5. Update from `main.c` inside `lvgl_port_lock(0)` / `lvgl_port_unlock()`

## Rules for AI Agents

1. **Never use `lv_obj_create(parent)`** as a panel/container — causes crash on this board.
2. **All widget updates** must be inside `lvgl_port_lock(0)` / `lvgl_port_unlock()`.
3. **Call `lv_obj_align_to()` after text changes** if using relative alignment.
4. **Theme init is mandatory** — always call `lv_theme_default_init()` before creating widgets.
