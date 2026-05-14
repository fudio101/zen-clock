# Deep Sleep Manager

> **[AI Context]** This component manages ESP32-S3 deep sleep for ZenClock.
> It provides inactivity auto-sleep, manual trigger, and wakeup configuration.
> Deep sleep draws ~6µA vs ~80mA active — essential for battery life.
> **On wake, the ESP32 performs a full restart.** RTC SRAM persists; regular SRAM does not.

## Architecture

```
components/deep_sleep/
├── include/deep_sleep.h  ← Public API
├── src/deep_sleep.c      ← esp_timer + FreeRTOS sleep task
└── CMakeLists.txt
```

### How It Works

```
deep_sleep_init()
  ├── xTaskCreate(sleep_task_fn)    ← blocks on ulTaskNotifyTake
  ├── esp_timer_create(inactivity)  ← one-shot timer
  └── esp_timer_start_once(timeout) ← if timeout_s > 0

Inactivity fires → inactivity_cb → deep_sleep_trigger()
Manual trigger  → deep_sleep_trigger()
Both buttons held ≥ 800ms → deep_sleep_trigger() [via app_handlers.c]

deep_sleep_trigger() → xTaskNotifyGive(sleep_task)

sleep_task wakes:
  ├── bsp_display_set_brightness(0, 1500ms)  ← fade out
  ├── vTaskDelay(1600ms)                     ← wait for fade
  ├── esp_sleep_enable_ext1_wakeup(GPIO0 | GPIO14, ANY_LOW)
  ├── esp_sleep_pd_config(RTC_PERIPH, ON)    ← keep RTC periph powered during sleep
  ├── rtc_gpio_pullup_en(GPIO0/GPIO14)       ← prevent pins floating LOW → immediate wake
  └── esp_deep_sleep_start()                 ← never returns
```

### Wakeup Sources

Both buttons wake the device (ext1, active-low):

| GPIO   | Button | Mask bit     |
|--------|--------|--------------|
| GPIO0  | BOOT   | `1ULL << 0`  |
| GPIO14 | IO14   | `1ULL << 14` |

Press either button to wake. The device performs a full restart and resumes the normal boot sequence.

## Public API

Header: [`include/deep_sleep.h`](include/deep_sleep.h)

| Function                               | Description                                                                                              |
|----------------------------------------|----------------------------------------------------------------------------------------------------------|
| `deep_sleep_init(timeout_s)`           | Initialize sleep manager. Creates task + timer. `0` = auto-sleep disabled. Call after `settings_init()`. |
| `deep_sleep_reset_timer()`             | Reset the inactivity countdown. Call on every button event. No-op if auto-sleep disabled.                |
| `deep_sleep_trigger()`                 | Trigger sleep immediately. Safe from any task or timer callback.                                         |
| `deep_sleep_update_timeout(timeout_s)` | Hot-update timeout after settings change. `0` = disable auto-sleep.                                      |

## Usage

```c
#include "deep_sleep.h"

// In app_main, after settings_init():
uint32_t sleep_s = (uint32_t)settings_get_sleep_h() * 3600
                 + (uint32_t)settings_get_sleep_m() * 60
                 + (uint32_t)settings_get_sleep_s();
deep_sleep_init(sleep_s);

// In button handler — reset timer on every press:
void on_button_press(int btn_id, bsp_btn_event_t event) {
    deep_sleep_reset_timer();
    // ... nav handling ...
}

// When user changes Sleep H/M/S in settings:
deep_sleep_update_timeout(new_sleep_s);

// Manual trigger (e.g. "Sleep Now" menu item):
deep_sleep_trigger();
```

## Sleep Timeout Configuration

Sleep timeout is stored in NVS (namespace `zenclock`):

| NVS Key   | Range  | Default | Settings item |
|-----------|--------|---------|---------------|
| `sleep_h` | 0–23 h | 0       | Sleep H       |
| `sleep_m` | 0–59 m | 0       | Sleep M       |
| `sleep_s` | 0–59 s | 0       | Sleep S       |

All three set to `0` disables auto-sleep entirely.

## Rules for AI Agents

1. **Call `deep_sleep_init()` once at boot**, after `settings_init()` and before `bsp_buttons_init()`.
2. **Always call `deep_sleep_reset_timer()` at the top of `on_button_press()`** — before any other logic.
3. **Never call `esp_deep_sleep_start()` directly** — always use `deep_sleep_trigger()` so the backlight fades first.
4. **`deep_sleep_trigger()` is non-blocking** — it notifies the sleep task via FreeRTOS notification. The caller returns
   immediately.
5. **After deep sleep wake, all FreeRTOS state is gone** — the device does a full restart from `app_main`.
