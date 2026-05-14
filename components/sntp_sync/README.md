# SNTP Sync Component

## 1. Overview

The `sntp_sync` component provides NTP-based time synchronization for the ZenClock firmware running on the ESP32-S3.
It handles the initial sync and automatic periodic re-synchronization (default: every 1 hour) to combat
ESP32 RTC drift, using primary (VNIX) and fallback (Pool NTP, Google) servers.

On wake from deep sleep, the component checks `RTC_DATA_ATTR` memory for the last successful sync timestamp.
If the elapsed time is within the 1-hour resync interval, the initial NTP sync is skipped and `SNTP_EVENT_SYNCED`
is reported immediately — reducing reconnect latency after short sleep cycles.

## 2. API Summary

```c
// Initialize and start SNTP sync. Call after WiFi is connected.
esp_err_t sntp_sync_init(sntp_sync_event_cb_t cb);

// Stop SNTP sync and release resources.
void sntp_sync_deinit(void);
```

### Events

| Event               | Description                                                    |
|---------------------|----------------------------------------------------------------|
| `SNTP_EVENT_SYNCED` | Time successfully synchronized (or restored from RTC on wake). |
| `SNTP_EVENT_FAILED` | Sync attempt timed out (30s) with no response from any server. |

## 2b. Deep Sleep Wake Behavior

The last successful sync time is stored in RTC memory (`RTC_DATA_ATTR`) which persists through deep sleep:

| Condition on wake                    | Behavior                                                                                                      |
|--------------------------------------|---------------------------------------------------------------------------------------------------------------|
| Last sync < 1 hour ago               | Skip initial NTP sync, report `SNTP_EVENT_SYNCED` immediately. Wait remaining interval then re-sync normally. |
| Last sync ≥ 1 hour ago or first boot | Perform full initial sync (up to 30s wait).                                                                   |

> **Note:** The ESP32 RTC oscillator keeps running during deep sleep with ~72ms/hr drift. `time(NULL)` post-wake
> is accurate enough to evaluate the elapsed time correctly.

## 3. Configuration

| Parameter                | Default            | Description                                    |
|--------------------------|--------------------|------------------------------------------------|
| `SNTP_RESYNC_INTERVAL_S` | `3600`             | Seconds between periodic re-syncs.             |
| `SNTP_SYNC_TIMEOUT_S`    | `30`               | Max seconds to wait for initial sync response. |
| Primary server           | `time.vnix.gov.vn` | Primary NTP server.                            |
| Fallback server 1        | `pool.ntp.org`     | First fallback.                                |
| Fallback server 2        | `time.google.com`  | Second fallback.                               |

## 4. Usage

```c
static void on_sntp_sync(sntp_sync_event_t event) {
    if (event == SNTP_EVENT_SYNCED) {
        ESP_LOGI(TAG, "Time synchronized");
    }
}

// After WiFi connected:
sntp_sync_init(on_sntp_sync);
```

Call `sntp_sync_deinit()` before entering deep sleep or when WiFi is disconnected for an extended period.
