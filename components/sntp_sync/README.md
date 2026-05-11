# SNTP Sync Component

> **[AI Context & Instructions]**
> To any AI Agent or LLM reading this file:
> This is a local ESP-IDF component used to manage network time synchronization (SNTP) for the ZenClock project.
> It abstracts the modern `esp_netif_sntp` API into a thread-safe, non-blocking background task.
>
> **AI Rules for using this component:**
> 1. Do NOT use the deprecated `esp_sntp_*` API directly in the application. Use the functions provided by `sntp_sync.h`.
> 2. This component spawns its own FreeRTOS task to prevent blocking the main thread during network latency.
> 3. Use the `sntp_sync_cb_t` callback to update UI elements (like status icons) based on sync state transitions.

---

## 1. Overview
This component is designed for ZenClock application developers who need to maintain accurate time synchronization via the network. It handles the initial sync and automatic periodic re-synchronization (default: every 1 hour) to combat ESP32 RTC drift, using primary (VNIX) and fallback (Pool NTP, Google) servers.

## 2. API Summary
Include the header in your C files:
```c
#include "sntp_sync.h"
```

**Key Data Types:**
* `sntp_sync_event_t`: Enum defining the sync states (`SNTP_EVENT_SYNCING`, `SNTP_EVENT_SYNCED`, `SNTP_EVENT_FAILED`).
* `sntp_sync_cb_t`: Callback function type for state changes `void (*sntp_sync_cb_t)(sntp_sync_event_t event)`.

**Key Functions:**
* `esp_err_t sntp_sync_start(sntp_sync_cb_t on_sync);`
  * Starts the non-blocking background task for SNTP synchronization.
* `bool sntp_sync_is_synced(void);`
  * Returns `true` if the time has been successfully synchronized.
* `void sntp_sync_stop(void);`
  * Stops the SNTP client, kills the background task, and frees resources.

## 3. Configuration (Menuconfig)
For fallback servers to work, configure LWIP in ESP-IDF `menuconfig`:
* `Component config` → `LWIP` → `SNTP`
* Set **Maximum number of NTP servers** to at least `3`.

## 4. Usage Example (for AI and Humans)

```c
#include "sntp_sync.h"
#include <esp_log.h>

static const char *TAG = "APP";

// 1. Define a callback to handle state changes
static void on_sntp_event(sntp_sync_event_t event) {
    switch (event) {
        case SNTP_EVENT_SYNCING:
            ESP_LOGI(TAG, "NTP Syncing...");
            // Show syncing icon in UI
            break;
        case SNTP_EVENT_SYNCED:
            ESP_LOGI(TAG, "NTP Synced!");
            // Update clock UI, hide syncing icon
            break;
        case SNTP_EVENT_FAILED:
            ESP_LOGW(TAG, "NTP Sync Failed!");
            // Show error icon in UI
            break;
    }
}

void app_main(void) {
    // Wait for network connection first...
    
    // 2. Start the sync process (non-blocking)
    sntp_sync_start(on_sntp_event);

    // 3. Later, check status if needed before using time
    if (sntp_sync_is_synced()) {
        // Safe to read system time
    }
}
```
