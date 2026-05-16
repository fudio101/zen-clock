# UI Component

LVGL-based UI for ZenClock. Thin public surface: callers only touch `ui.h` and `nav.h`.

## Architecture

```
app_handlers.c
      │
      ▼
   ui_init()          ← lvgl theme + nav_init()
      │
      ▼
   nav.c              ← navigation state machine
   ├── clock screen   → clock_face_text.c + status_bar.c
   ├── menu screen    → menu_screen.c + status_bar.c
   └── settings screen→ settings_screen.c + status_bar.c
                              ↕ (overlay)
                         prov_screen.c
```

Nav owns all screen creation and teardown. Nothing outside `nav.c` calls `clock_face_create`, `status_bar_create`, or
the other widget constructors.

## File Organization

```
ui/
├── ui.h / ui.c                           ← Public entry point + theme state
├── nav.h / nav.c                         ← Navigation state machine
├── clock_face.h / clock_face_text.c      ← Clock face widget (DS-Digital font)
├── status_bar.h / status_bar.c           ← Status bar widget
├── prov_screen.h / prov_screen.c         ← BLE provisioning overlay
├── menu_screen.h / menu_screen.c         ← Menu screen
├── settings_screen.h / settings_screen.c ← Settings screen with inline edit
├── fonts/
│   ├── DS-DIGIT.TTF                      ← DS-Digital source font (7-segment style)
│   ├── DS-DIGI.TTF / DS-DIGIB.TTF / DS-DIGII.TTF ← other weights (unused)
│   ├── DIGITAL.TXT                       ← font license
│   ├── lv_font_ds_digital_48.h           ← generated font declaration
│   └── lv_font_ds_digital_48.c           ← generated LVGL bitmap (4bpp, size 48)
└── CMakeLists.txt
```

The clock face is swappable: replace `clock_face_text.c` with another implementation in `CMakeLists.txt` without
touching any header.

### Regenerating the font

```bash
lv_font_conv --bpp 4 --size 48 \
  --font components/ui/fonts/DS-DIGIT.TTF \
  --range 0x30-0x39,0x3A,0x2F,0x20 \
  --format lvgl \
  --output components/ui/fonts/lv_font_ds_digital_48.c \
  --no-compress
```

After regenerating, replace the `#ifdef LV_LVGL_H_INCLUDE_SIMPLE … #endif` include block at the top with a plain `#include "lvgl.h"`.

## Public APIs

### `ui.h` — entry point

```c
void ui_init(bool is_light);             // init LVGL theme + shared bg style, call nav_init()
void ui_set_theme(bool is_light);        // switch light/dark theme; updates bg on all screens instantly
bool ui_is_light_theme(void);            // query current theme state
void ui_apply_screen_bg(lv_obj_t *scr); // attach shared bg style to a new screen object
```

Screen background colors are managed via a shared `lv_style_t` so `ui_set_theme()` calls
`lv_obj_report_style_change()` once and LVGL refreshes all screens automatically — no manual
per-screen updates needed.

**Theme color palette:**

| Element      | Light (`#e6e6e6` bg) | Dark (`#0d0d0d` bg) |
|--------------|----------------------|---------------------|
| Clock digits | `#333333`            | `#01ddff`           |
| Date label   | `#666666`            | `#007a99`           |
| Screen bg    | `#e6e6e6`            | `#0d0d0d`           |

### `nav.h` — navigation

```c
typedef enum {
    NAV_ACTION_UP,      // BOOT short — move up / increase
    NAV_ACTION_DOWN,    // IO14 short — move down / decrease
    NAV_ACTION_SELECT,  // BOOT long  — select / enter
    NAV_ACTION_BACK,    // IO14 long  — back
} nav_action_t;

typedef void (*nav_action_cb_t)(void);

void nav_init(void);
void nav_handle_action(nav_action_t action);
void nav_register_reset_wifi_cb(nav_action_cb_t cb);
void nav_register_sleep_cb(nav_action_cb_t cb);
```

### `clock_face.h` — clock face widget

```c
void clock_face_create(lv_obj_t *parent); // HH:MM:SS + DD/MM/YYYY, 1s LVGL timer
void clock_face_destroy(void);            // stop timer before deleting parent
```

### `status_bar.h` — status bar widget

```c
void status_bar_create(lv_obj_t *parent);
void status_bar_destroy(void);
void status_bar_set_wifi_status(wifi_status_t status); // includes WIFI_STATUS_PROVISIONING
void status_bar_set_sntp_status(sntp_status_t status);
```

Internal 30-second LVGL timer refreshes battery level automatically.

### `prov_screen.h` — BLE provisioning overlay

```c
void prov_screen_show(const char *device_name, const char *password); // full-screen QR overlay
void prov_screen_hide(void);                    // remove overlay, reveal clock
```

### `menu_screen.h`

```c
void menu_screen_create(lv_obj_t *parent);
void menu_screen_focus_prev(void);   // stops at first item
void menu_screen_focus_next(void);   // stops at last item
int  menu_screen_get_focus(void);
void menu_screen_set_focus(int index);
```

### `settings_screen.h`

```c
void settings_screen_create(lv_obj_t *parent);
void settings_screen_focus_prev(void);
void settings_screen_focus_next(void);
int  settings_screen_get_focus(void);
void settings_screen_set_focus(int index);
bool settings_screen_is_action_item(int index);
void settings_screen_execute_action(int index, nav_action_cb_t cb);
void settings_screen_enter_edit(int index);
void settings_screen_exit_edit(void);
void settings_screen_edit_increase(void);
void settings_screen_edit_decrease(void);
```

## Navigation Flow

```
Clock screen
  └─ any long press ──────────────────────→ Menu screen
                                               ├─ UP/DOWN: navigate items
                                               ├─ SELECT:  enter Settings
                                               └─ BACK:    → Clock screen

Settings screen (7 items: Theme, Brightness, Sleep H/M/S, Sleep Now, Reset WiFi)
  ├─ UP/DOWN: navigate items (scrollable — 5 visible at a time)
  ├─ SELECT:  enter edit mode (TOGGLE/RANGE) or execute action (ACTION items)
  └─ BACK:    → Menu screen

Settings edit mode (TOGGLE/RANGE items only)
  ├─ UP/DOWN: change value (auto-saved to NVS + applied live)
  ├─ SELECT:  exit edit mode
  └─ BACK:    exit edit mode

Action items (Sleep Now, Reset WiFi): SELECT fires callback, no edit mode
```

## Key Constraints

- **LVGL timers only** for UI updates — never FreeRTOS tasks.
- **Shared styles for theme colors** — screen bg uses a single `lv_style_t`; inline `lv_obj_set_style_*` calls per-object won't auto-update on theme switch.
- **LVGL lock required** — all external LVGL calls need `lvgl_port_lock(0)` / `lvgl_port_unlock()`.
- **Nav owns screen lifecycle** — do not call widget constructors from outside `nav.c`. Always call `ui_apply_screen_bg(scr)` after `lv_obj_create(NULL)` in `nav.c`.
- **Clock face is swappable** — swap `clock_face_text.c` in `CMakeLists.txt`; `clock_face.h` interface stays the same.
- **Clock text colors** are set at `clock_face_create()` time via `ui_is_light_theme()` — they update on next screen creation after a theme change, not live.
