// SPDX-License-Identifier: MIT
// ZenClock — SNTP time synchronization implementation
//
// menuconfig required:
//   Component config → LWIP → SNTP:
//     - Maximum number of NTP servers = 3
//     (optional) Request NTP servers from DHCP = Yes

#include "sntp_sync.h"

#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "SNTP";

#define SNTP_RESYNC_INTERVAL_S 3600 // Re-sync every 1 hour (ESP32 RTC drift ~72ms/hr)

static bool s_synced = false;
static sntp_sync_cb_t s_on_sync = NULL;
static TaskHandle_t s_sntp_task = NULL;

// ============================================================
// Notification callback — fired by SNTP internally on time set
// ============================================================
static void time_sync_notification_cb(struct timeval *tv)
{
  ESP_LOGI(TAG, ">>> NTP time received! tv_sec=%lld", (long long)tv->tv_sec);
}

// ============================================================
// Log current synchronized time
// ============================================================
static void log_synced_time(void)
{
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  ESP_LOGI(TAG, "Time synchronized: %s", buf);
}

// ============================================================
// Wait for SNTP sync with timeout (used for both initial and re-sync)
// Returns true if synced, false if timed out.
// ============================================================
static bool wait_for_sync(int max_retries)
{
  for (int retry = 0; retry < max_retries; retry++)
  {
    esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(2000));
    if (ret == ESP_OK)
    {
      return true;
    }
    ESP_LOGI(TAG, "Sync poll %d/%d (ret=%s)", retry + 1, max_retries, esp_err_to_name(ret));
  }
  return false;
}

// ============================================================
// SNTP task — initial sync + periodic re-sync
// ============================================================
static void sntp_task(void *arg)
{
  // --- Initial sync ---
  if (s_on_sync)
  {
    s_on_sync(SNTP_EVENT_SYNCING);
  }
  ESP_LOGI(TAG, "Waiting for NTP time sync...");

  bool ok = wait_for_sync(15); // 15 × 2s = 30s max wait
  if (ok)
  {
    s_synced = true;
    log_synced_time();
  }
  else
  {
    ESP_LOGW(TAG, "NTP sync timeout after 30 seconds");
  }

  // Fire callback for initial sync result
  if (s_on_sync)
  {
    s_on_sync(ok ? SNTP_EVENT_SYNCED : SNTP_EVENT_FAILED);
  }

  // --- Periodic re-sync loop ---
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(SNTP_RESYNC_INTERVAL_S * 1000));

    // Notify: re-sync starting
    if (s_on_sync)
    {
      s_on_sync(SNTP_EVENT_SYNCING);
    }

    ESP_LOGI(TAG, "Re-syncing NTP (interval=%ds)...", SNTP_RESYNC_INTERVAL_S);
    esp_sntp_restart();

    ok = wait_for_sync(5); // 5 × 2s = 10s max for re-sync
    if (ok)
    {
      s_synced = true;
      log_synced_time();
    }
    else
    {
      ESP_LOGW(TAG, "NTP re-sync timeout");
      s_synced = false;
    }

    if (s_on_sync)
    {
      s_on_sync(ok ? SNTP_EVENT_SYNCED : SNTP_EVENT_FAILED);
    }
  }
}

// ============================================================
// Public API
// ============================================================

esp_err_t sntp_sync_start(sntp_sync_cb_t on_sync)
{
  s_on_sync = on_sync;
  s_synced = false;

  // Small delay to let DNS from DHCP propagate
  vTaskDelay(pdMS_TO_TICKS(500));

  // Use the modern esp_netif_sntp API (ESP-IDF 5.1+ / 6.0)
  // Old esp_sntp_* API is NOT thread-safe per official docs.
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_PRIMARY_SERVER);
  config.smooth_sync = false; // Immediate time set (not gradual adjust)
  config.sync_cb = time_sync_notification_cb;
  // Accept NTP from DHCP only if enabled in menuconfig
  // Path: Component config → LWIP → SNTP → Request NTP servers from DHCP
#if CONFIG_LWIP_DHCP_GET_NTP_SRV
  config.server_from_dhcp = true;
#endif

  esp_err_t ret = esp_netif_sntp_init(&config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_netif_sntp_init failed: %s", esp_err_to_name(ret));
    return ret;
  }

  // Log configured servers
  // Note: CONFIG_LWIP_SNTP_MAX_SERVERS must be >= 3 in menuconfig
  // for fallback servers to take effect.
  // Path: Component config → LWIP → SNTP → Maximum number of NTP servers
  ESP_LOGI(TAG, "SNTP server [0]: %s", SNTP_PRIMARY_SERVER);

#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
  esp_sntp_setservername(1, "pool.ntp.org");
  ESP_LOGI(TAG, "SNTP server [1]: pool.ntp.org");
#endif
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 2
  esp_sntp_setservername(2, "time.google.com");
  ESP_LOGI(TAG, "SNTP server [2]: time.google.com");
#endif

#if CONFIG_LWIP_SNTP_MAX_SERVERS <= 1
  ESP_LOGW(TAG, "Only 1 NTP server slot! Set CONFIG_LWIP_SNTP_MAX_SERVERS=3 in menuconfig");
#endif

  // Spawn persistent SNTP task (initial sync + periodic re-sync)
  BaseType_t xret = xTaskCreate(sntp_task, "sntp_sync", 3072, NULL, 2, &s_sntp_task);

  return (xret == pdPASS) ? ESP_OK : ESP_FAIL;
}

bool sntp_sync_is_synced(void)
{
  return s_synced;
}

void sntp_sync_stop(void)
{
  // Delete the persistent task first
  if (s_sntp_task)
  {
    vTaskDelete(s_sntp_task);
    s_sntp_task = NULL;
  }

  esp_netif_sntp_deinit();
  s_synced = false;
  ESP_LOGI(TAG, "SNTP client stopped");
}
