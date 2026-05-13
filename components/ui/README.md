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
├── ui.h / ui.c                           ← Public entry point
├── nav.h / nav.c                         ← Navigation state machine
├── clock_face.h / clock_face_text.c      ← Clock face widget (text style)
├── status_bar.h / status_bar.c           ← Status bar widget
├── prov_screen.h / prov_screen.c         ← BLE provisioning overlay
├── menu_screen.h / menu_screen.c         ← Menu screen
├── settings_screen.h / settings_screen.c ← Settings screen with inline edit
└── CMakeLists.txt
```

The clock face is swappable: replace `clock_face_text.c` with another implementation in `CMakeLists.txt` without
touching any header.

## Public APIs

### `ui.h` — entry point

```c
void ui_init(void);           // init LVGL theme, call nav_init()
void ui_set_theme(bool dark); // switch light/dark theme
```

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
void status_bar_set_sntp_status(bool synced);
```

Internal 30-second LVGL timer refreshes battery level automatically.

### `prov_screen.h` — BLE provisioning overlay

```c
void prov_screen_show(const char *device_name); // full-screen QR overlay
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

Settings screen
  ├─ UP/DOWN: navigate items
  ├─ SELECT:  enter edit mode (or execute action item)
  └─ BACK:    → Menu screen

Settings edit mode (non-action items)
  ├─ UP/DOWN: change value (auto-saved to NVS)
  ├─ SELECT:  exit edit mode
  └─ BACK:    exit edit mode
```

## Key Constraints

- **LVGL timers only** for UI updates — never FreeRTOS tasks.
- **No hardcoded colors** — use theme-aware LVGL styles (light/dark support).
- **LVGL lock required** — all external LVGL calls need `lvgl_port_lock(0)` / `lvgl_port_unlock()`.
- **Nav owns screen lifecycle** — do not call widget constructors from outside `nav.c`.
- **Clock face is swappable** — swap `clock_face_text.c` in `CMakeLists.txt`; `clock_face.h` interface stays the same.
