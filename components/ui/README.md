# UI — Modular LVGL Interface

A modular, theme-aware LVGL-based user interface for ZenClock built on widget components.

## Overview

The UI component manages the main display using LVGL, organized around **discrete, reusable widget modules**:

- **Clock Face** — Renders time with configurable face styles
- **Status Bar** — Displays WiFi, battery, and theme indicators
- **Provisioning Screen** — Full-screen QR code overlay for BLE setup

All widgets work within a unified theming system (light/dark) and integrate seamlessly with the LVGL port lock for thread-safe updates.

## Architecture

### Component Layers

```
┌─────────────────────────────────────────────────────┐
│ Application (main.c)                                │
│ - Receives WiFi/settings events                    │
│ - Calls ui_init(), ui_set_theme()                  │
│ - Calls prov_screen_show/hide() on BLE events      │
└──────────────────────┬────────────────────────────┘
                       │ (lvgl_port_lock required)
┌──────────────────────┴────────────────────────────┐
│ UI Module (ui.h, ui.c)                             │
│ - Main screen + container                         │
│ - Delegates to widget modules                      │
│ - Theme management                                 │
└───────────────────┬──────────────┬────────────────┘
                    │              │
        ┌───────────┴──────┐   ┌───┴───────────────┐
        │                  │   │                   │
   ┌────┴──────┐    ┌──────┴───┴──┐        ┌──────┴────┐
   │ Clock Face│    │ Status Bar  │        │Prov Screen│
   │(widget)   │    │ (widget)    │        │(overlay)  │
   └───────────┘    └─────────────┘        └───────────┘
```

**Data Flow:**

1. External event (WiFi connected, theme changed) → callback fires
2. Callback calls `lvgl_port_lock(0)`
3. UI updates via widget APIs: `status_bar_set_wifi_status()`, `prov_screen_show()`, etc.
4. Callback calls `lvgl_port_unlock()`
5. LVGL redraws on next event loop tick

### File Organization

```
ui/
├── ui.h                    ← Main API: ui_init(), ui_set_theme()
├── ui.c                    ← Screen setup, theme switching, widget delegation
├── clock_face.h            ← Clock widget API
├── clock_face.c            ← Clock rendering (text/numeric faces)
├── status_bar.h            ← Status bar API (WiFi, battery, theme icons)
├── status_bar.c            ← Status bar implementation
├── prov_screen.h           ← Provisioning overlay API
├── prov_screen.c           ← QR code + setup UI overlay
└── CMakeLists.txt
```

## Widget Modules

### Clock Face (`clock_face.h`)

Renders the current time with a configurable display style.

**API:**

```c
/**
 * @brief Create and attach clock face to the given container.
 * @param parent LVGL container (typically main screen)
 * @param is_light true for light theme, false for dark theme
 */
void clock_face_init(lv_obj_t *parent, bool is_light);

/**
 * @brief Update clock display with current time.
 * @param hours Hours (0-23)
 * @param minutes Minutes (0-59)
 * @param seconds Seconds (0-59)
 */
void clock_face_update(uint8_t hours, uint8_t minutes, uint8_t seconds);

/**
 * @brief Switch clock face to light or dark theme.
 * @param is_light true for light theme colors
 */
void clock_face_set_theme(bool is_light);
```

**Layout:**

Clock face centers on the main screen with large, legible time display. Face style is configurable (e.g., HH:MM:SS text format, numeric digits, etc.).

### Status Bar (`status_bar.h`)

Top-of-screen indicator bar showing WiFi status, battery level, and theme indicator.

**API:**

```c
/**
 * @brief Create and attach status bar to the given container.
 * @param parent LVGL container (typically main screen)
 * @param is_light true for light theme, false for dark theme
 */
void status_bar_init(lv_obj_t *parent, bool is_light);

/**
 * @brief Update WiFi status indicator.
 * @param status wifi_status_t enum (CONNECTED, DISCONNECTED, PROVISIONING, etc.)
 */
void status_bar_set_wifi_status(wifi_status_t status);

/**
 * @brief Update battery level indicator.
 * @param percent Battery percentage (0-100)
 */
void status_bar_set_battery(uint8_t percent);

/**
 * @brief Switch status bar to light or dark theme.
 * @param is_light true for light theme colors
 */
void status_bar_set_theme(bool is_light);
```

**WiFi Status Values:**

| Status | Icon | When |
|--------|------|------|
| `WIFI_STATUS_DISCONNECTED` | ✕ (gray) | No WiFi connection |
| `WIFI_STATUS_CONNECTING` | ◐ (animating) | Scanning/connecting |
| `WIFI_STATUS_CONNECTED` | ✓ (color) | WiFi + internet verified |
| `WIFI_STATUS_PROVISIONING` | ◆ (cyan/Bluetooth) | BLE provisioning active |

**Theme Indicator:**

A small circle in the status bar shows current theme: light (sun icon) or dark (moon icon).

### Provisioning Screen (`prov_screen.h`)

Full-screen QR code overlay shown during BLE WiFi provisioning setup.

**API:**

```c
/**
 * @brief Show BLE provisioning overlay with QR code.
 * Creates full-screen container on lv_screen_active() with:
 * - 140x140 QR code (left side, 12px padding)
 * - "WiFi Setup" title, instructions, device name (right side)
 * Safe to call multiple times (guards against double-create).
 * Must be called within lvgl_port_lock() / lvgl_port_unlock().
 * @param device_name BLE advertisement name (e.g. "PROV_ZenClock_A1B2")
 */
void prov_screen_show(const char *device_name);

/**
 * @brief Hide provisioning overlay, revealing clock screen beneath.
 * Deletes overlay container and all children.
 * Must be called within lvgl_port_lock() / lvgl_port_unlock().
 */
void prov_screen_hide(void);
```

**QR Code Content:**

QR code encodes a JSON payload compatible with Espressif BLE Prov:

```json
{
  "ver": "v1",
  "name": "<device_name>",
  "pop": "",
  "transport": "ble"
}
```

Example: If device_name is `"PROV_ZenClock_A1B2"`, QR encodes the above with that device name.

**Layout:**

```
┌─────────────────────────────────────────────────────┐
│ ┌──────────────────────────────────────────────────┐│
│ │                   Provisioning Screen             ││
│ │                                                   ││
│ │ ┌──────────┐  WiFi Setup                         ││
│ │ │          │                                      ││
│ │ │  QR Code │  Scan this QR with the              ││
│ │ │ 140×140  │  Espressif BLE Provisioning app     ││
│ │ │          │                                      ││
│ │ └──────────┘  Device:                            ││
│ │              PROV_ZenClock_A1B2                   ││
│ │                                                   ││
│ └──────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────┘
```

- **Black overlay** across entire screen with semi-transparency
- **Left side:** 140×140 QR code with 12px padding
- **Right side:** Setup instructions, device name label
- **Center-aligned** vertically

## Main API

### Initialization

```c
/**
 * @brief Initialize LVGL UI.
 * Creates main screen, clock face widget, and status bar widget.
 * Must be called while holding lvgl_port_lock.
 * @param is_light true to start in light theme, false for dark theme
 */
void ui_init(bool is_light);
```

### Theme Management

```c
/**
 * @brief Switch UI theme dynamically.
 * Updates all widgets to use light or dark color scheme.
 * Must be called while holding lvgl_port_lock.
 * @param is_light true for light theme, false for dark theme
 */
void ui_set_theme(bool is_light);
```

## Constraints & Usage Rules

### LVGL Port Lock

All UI operations must be wrapped in the LVGL port lock to ensure thread safety:

```c
// From WiFi callback or other non-LVGL thread:
lvgl_port_lock(0);

// Safe to call UI functions here:
status_bar_set_wifi_status(WIFI_STATUS_CONNECTED);
clock_face_update(14, 30, 45);
prov_screen_show("PROV_ZenClock_A1B2");

lvgl_port_unlock();
```

### Provisioning Screen Lifecycle

1. **Show during BLE provisioning:**
   - Call `prov_screen_show(device_name)` when `ble_provisioning_start()` is called
   - QR code displays setup instructions

2. **Hide on provisioning complete:**
   - Call `prov_screen_hide()` after BLE provisioning succeeds or times out
   - Reveals clock screen beneath

3. **Thread safety:**
   - Always call within `lvgl_port_lock()` / `lvgl_port_unlock()`
   - Safe to call `prov_screen_show()` multiple times (no-op if already shown)

### Color & Typography

Widgets use CSS custom properties (design tokens) defined in the LVGL theme:

- **Light theme:** Light background, dark text
- **Dark theme:** Dark background, light text
- **Accent colors:** Cyan for status, green for success, red for errors

All colors are defined in LVGL style data, not hardcoded.

## CMakeLists Integration

```cmake
idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS "."
    SRCS "ui.c" "clock_face.c" "status_bar.c" "prov_screen.c"
    PRIV_INCLUDE_DIRS "priv_include"
)
```

To extend with custom clock faces, add new `.c` files to `SRCS` and include the header in `ui.c`.

## Integration with Other Components

### WiFi Manager Integration

WiFi Manager fires events in a background task. Handle in callback:

```c
static void on_wifi_event(wifi_mgr_event_t event) {
    lvgl_port_lock(0);
    
    switch (event) {
        case WIFI_MGR_CONNECTED:
            status_bar_set_wifi_status(WIFI_STATUS_CONNECTED);
            break;
        case WIFI_MGR_DISCONNECTED:
            status_bar_set_wifi_status(WIFI_STATUS_DISCONNECTED);
            break;
        case WIFI_MGR_CONNECTING:
            status_bar_set_wifi_status(WIFI_STATUS_CONNECTING);
            break;
        // ... other cases ...
    }
    
    lvgl_port_unlock();
}
```

### BLE Provisioning Integration

Show/hide provisioning screen based on provisioning lifecycle:

```c
static void on_ble_prov_event(ble_prov_event_t event) {
    lvgl_port_lock(0);
    
    if (event == BLE_PROV_START) {
        prov_screen_show("PROV_ZenClock_A1B2");
        status_bar_set_wifi_status(WIFI_STATUS_PROVISIONING);
    } else if (event == BLE_PROV_COMPLETE || event == BLE_PROV_TIMEOUT) {
        prov_screen_hide();
        status_bar_set_wifi_status(WIFI_STATUS_DISCONNECTED);
    }
    
    lvgl_port_unlock();
}
```

### Settings Integration

Settings (theme preference, brightness) update UI dynamically:

```c
// After settings_get_theme() returns a value:
lvgl_port_lock(0);
ui_set_theme(is_light);
lvgl_port_unlock();
```

### Time Update Integration

Clock face updates on time ticks (e.g., every second from SNTP sync or timer):

```c
static void on_time_update(uint8_t h, uint8_t m, uint8_t s) {
    lvgl_port_lock(0);
    clock_face_update(h, m, s);
    lvgl_port_unlock();
}
```

## Testing & Debugging

- **Visual inspection:** Test themes (light/dark) on real hardware
- **Provisioning flow:** Verify QR code displays and BLE setup completes
- **State transitions:** Test WiFi connect/disconnect with status bar updates
- **No lock violations:** Ensure all UI calls are wrapped in `lvgl_port_lock()`

## Performance Notes

- LVGL rendering is optimized for small changes (dirty area redraws only)
- Clock face updates every second (minimal impact)
- Status bar updates only on WiFi/battery changes
- Provisioning overlay is created/destroyed infrequently
- Theme switching redraws entire screen once (acceptable on startup/user preference change)
