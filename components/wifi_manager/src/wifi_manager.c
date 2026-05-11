// SPDX-License-Identifier: MIT
// ZenClock WiFi Manager — State-machine driven, persistent-task architecture
//
// Features:
//   - 6-state machine: IDLE → SCANNING → CONNECTING → VERIFYING → CONNECTED
//   - Infinite retry with exponential backoff (never gives up)
//   - Reconnect: retry same SSID 3×, then full re-scan
//   - DNS probe after Got IP to verify internet connectivity
//   - Lightweight event handler (bits only, zero logic)

#include "wifi_manager.h"
#include "wifi_priv.h"

#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <lwip/netdb.h>

static const char *TAG = "WiFiMgr";

// ============================================================
// Event group bits — set by event handler, consumed by task
// ============================================================
#define BIT_GOT_IP BIT0
#define BIT_FAIL BIT1
#define BIT_STA_START BIT2
#define BIT_DISCONNECTED BIT3
#define BIT_STOP BIT4

// ============================================================
// Module state
// ============================================================
static EventGroupHandle_t s_event_group = NULL;
static SemaphoreHandle_t s_mutex = NULL;
static TaskHandle_t s_task_handle = NULL;
static wifi_event_cb_t s_callback = NULL;
static wifi_state_t s_state = WIFI_ST_IDLE;
static ssid_map_t s_cred_map;
static int s_cred_count = 0;
static esp_netif_t *s_sta_netif = NULL;
static char s_connected_ssid[SSID_MAX_LEN] = {0};
static int s_scan_attempt = 0;

// Candidate list (shared between SCANNING and CONNECTING states)
static wifi_ap_record_t *s_ap_list = NULL;
static int s_candidates[WIFI_MAX_CREDENTIALS];
static int s_candidate_count = 0;

// ============================================================
// State helpers (thread-safe)
// ============================================================

static const char *state_name(wifi_state_t st)
{
  static const char *names[] = {"IDLE", "SCANNING", "CONNECTING", "VERIFYING", "CONNECTED", "RECONNECTING"};
  return (st <= WIFI_ST_RECONNECTING) ? names[st] : "?";
}

static wifi_state_t get_state(void)
{
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  wifi_state_t st = s_state;
  xSemaphoreGive(s_mutex);
  return st;
}

static void set_state(wifi_state_t new_state)
{
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  wifi_state_t old = s_state;
  s_state = new_state;
  xSemaphoreGive(s_mutex);
  if (old != new_state)
  {
    ESP_LOGI(TAG, "[%s → %s]", state_name(old), state_name(new_state));
  }
}

// ============================================================
// Callback helper
// ============================================================
static void fire_event(wifi_manager_event_t event)
{
  if (s_callback)
  {
    s_callback(event);
  }
}

// ============================================================
// Backoff delay calculator
// ============================================================
static uint32_t calc_backoff_ms(int attempt, uint32_t base_ms, uint32_t max_ms)
{
  uint32_t delay = base_ms;
  for (int i = 0; i < attempt && delay < max_ms; i++)
  {
    delay <<= 1;
  }
  return (delay < max_ms) ? delay : max_ms;
}

// ============================================================
// Check if stop was requested (non-blocking)
// ============================================================
static bool check_stop_signal(void)
{
  EventBits_t bits = xEventGroupGetBits(s_event_group);
  if (bits & BIT_STOP)
  {
    xEventGroupClearBits(s_event_group, BIT_STOP | BIT_GOT_IP | BIT_FAIL | BIT_DISCONNECTED);
    ESP_LOGI(TAG, "Stop signal received");
    set_state(WIFI_ST_IDLE);
    return true;
  }
  return false;
}

// ============================================================
// Event handler — ONLY sets bits, zero logic
// ============================================================
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
  if (base == WIFI_EVENT)
  {
    switch (id)
    {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "STA interface started");
      xEventGroupSetBits(s_event_group, BIT_STA_START);
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
    {
      wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)data;
      ESP_LOGW(TAG, "Disconnected (reason=%d)", evt->reason);
      xEventGroupSetBits(s_event_group, BIT_DISCONNECTED);
      break;
    }

    default:
      break;
    }
  }
  else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
    xEventGroupSetBits(s_event_group, BIT_GOT_IP);
  }
}

// ============================================================
// Compare function for sorting AP records by RSSI (descending)
// ============================================================
static int rssi_compare(const void *a, const void *b)
{
  const wifi_ap_record_t *ap_a = (const wifi_ap_record_t *)a;
  const wifi_ap_record_t *ap_b = (const wifi_ap_record_t *)b;
  return ap_b->rssi - ap_a->rssi;
}

// ============================================================
// Aggregated scan — run multiple rounds, merge by BSSID
// Returns number of unique APs in the merged buffer.
// ============================================================
static int do_aggregated_scan(wifi_ap_record_t *merged, int max_aps)
{
  int total = 0;

  for (int round = 0; round < SCAN_ROUNDS; round++)
  {
    ESP_LOGI(TAG, "Scan round %d/%d...", round + 1, SCAN_ROUNDS);

    wifi_scan_config_t scan_cfg = {
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = SCAN_MIN_TIME_MS,
        .scan_time.active.max = SCAN_MAX_TIME_MS,
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_cfg, true);
    if (ret != ESP_OK)
    {
      ESP_LOGW(TAG, "Scan round %d failed: %s", round + 1, esp_err_to_name(ret));
      continue;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(TAG, "  Round %d: %d APs", round + 1, ap_count);

    if (ap_count == 0)
    {
      continue;
    }

    wifi_ap_record_t *round_aps = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (!round_aps)
    {
      ESP_LOGW(TAG, "  Failed to allocate round buffer, skipping");
      // Must still consume scan results to free internal ESP-IDF buffer
      esp_wifi_scan_get_ap_records(&ap_count, NULL);
      continue;
    }
    esp_wifi_scan_get_ap_records(&ap_count, round_aps);

    // Merge: if BSSID already seen, keep best RSSI; otherwise append
    for (int i = 0; i < ap_count; i++)
    {
      bool found = false;
      for (int j = 0; j < total; j++)
      {
        if (memcmp(merged[j].bssid, round_aps[i].bssid, 6) == 0)
        {
          if (round_aps[i].rssi > merged[j].rssi)
          {
            merged[j] = round_aps[i];
          }
          found = true;
          break;
        }
      }
      if (!found && total < max_aps)
      {
        merged[total++] = round_aps[i];
      }
    }

    free(round_aps);

    if (round < SCAN_ROUNDS - 1)
    {
      vTaskDelay(pdMS_TO_TICKS(SCAN_INTER_DELAY_MS));
    }
  }

  if (total > 0)
  {
    qsort(merged, total, sizeof(wifi_ap_record_t), rssi_compare);
  }

  return total;
}

// ============================================================
// Try connecting to a single candidate AP
// Returns true if connected (got IP), false otherwise.
// ============================================================
static bool try_connect_candidate(const wifi_ap_record_t *ap, const char *password)
{
  const char *ssid = (const char *)ap->ssid;
  fire_event(WIFI_MGR_CONNECTING);

  wifi_config_t wifi_cfg = {0};
  size_t ssid_len = strlen(ssid);
  if (ssid_len > sizeof(wifi_cfg.sta.ssid))
  {
    ssid_len = sizeof(wifi_cfg.sta.ssid);
  }
  memcpy(wifi_cfg.sta.ssid, ssid, ssid_len);

  if (password && password[0] != '\0')
  {
    size_t pass_len = strlen(password);
    if (pass_len > sizeof(wifi_cfg.sta.password))
    {
      pass_len = sizeof(wifi_cfg.sta.password);
    }
    memcpy(wifi_cfg.sta.password, password, pass_len);
  }

  wifi_cfg.sta.bssid_set = false;
  wifi_cfg.sta.channel = 0;

  esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "set_config failed: %s", esp_err_to_name(ret));
    return false;
  }

  xEventGroupClearBits(s_event_group, BIT_GOT_IP | BIT_FAIL | BIT_DISCONNECTED);
  esp_wifi_connect();

  EventBits_t bits = xEventGroupWaitBits(s_event_group, BIT_GOT_IP | BIT_DISCONNECTED, pdTRUE, pdFALSE,
                                         pdMS_TO_TICKS(CONNECT_TIMEOUT_MS));

  if (bits & BIT_GOT_IP)
  {
    strncpy(s_connected_ssid, ssid, SSID_MAX_LEN - 1);
    s_connected_ssid[SSID_MAX_LEN - 1] = '\0';
    return true;
  }

  esp_wifi_disconnect();
  vTaskDelay(pdMS_TO_TICKS(500));
  return false;
}

// ============================================================
// DNS probe — verify internet connectivity after getting IP
// ============================================================
static bool do_dns_probe(void)
{
  ESP_LOGI(TAG, "Verifying internet connectivity...");
  fire_event(WIFI_MGR_GOT_IP);

  for (int probe = 0; probe < DNS_PROBE_MAX; probe++)
  {
    EventBits_t bits = xEventGroupGetBits(s_event_group);
    if (bits & (BIT_DISCONNECTED | BIT_STOP))
    {
      ESP_LOGW(TAG, "Connection lost during DNS probe");
      return false;
    }

    struct addrinfo hints = {.ai_family = AF_INET};
    struct addrinfo *res = NULL;
    int err = getaddrinfo(DNS_PROBE_HOST, "123", &hints, &res);
    if (err == 0 && res != NULL)
    {
      freeaddrinfo(res);
      ESP_LOGI(TAG, "DNS probe OK (attempt %d/%d)", probe + 1, DNS_PROBE_MAX);
      return true;
    }
    ESP_LOGD(TAG, "DNS probe %d/%d failed", probe + 1, DNS_PROBE_MAX);
    vTaskDelay(pdMS_TO_TICKS(DNS_PROBE_INTERVAL_MS));
  }

  ESP_LOGW(TAG, "DNS probe timed out — assuming connected");
  return true; // Fallback
}

// ============================================================
// Persistent WiFi task — state machine loop, never exits
// ============================================================
static void wifi_task(void *arg)
{
  // ── One-time setup ──

  // Load credentials from NVS into hash map
  s_cred_count = wifi_cred_store_load_all(&s_cred_map);
  ESP_LOGI(TAG, "=== Stored WiFi credentials (%d) ===", s_cred_count);
  for (int i = 0; i < SSID_MAP_CAPACITY; i++)
  {
    if (s_cred_map.buckets[i].occupied)
    {
      ESP_LOGI(TAG, "  SSID: \"%s\"  (pass: %s)", s_cred_map.buckets[i].ssid,
               s_cred_map.buckets[i].password[0] ? "***" : "<open>");
    }
  }

  // Start WiFi driver
  xEventGroupClearBits(s_event_group, BIT_STA_START);
  esp_err_t ret = esp_wifi_start();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_wifi_start failed: %s — task waiting for restart", esp_err_to_name(ret));
    for (;;)
    {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
  }
  xEventGroupWaitBits(s_event_group, BIT_STA_START, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));

  // Allocate merged AP buffer (reused across all scan cycles)
  s_ap_list = calloc(MAX_UNIQUE_APS, sizeof(wifi_ap_record_t));
  if (!s_ap_list)
  {
    ESP_LOGE(TAG, "Failed to allocate AP buffer — task halted");
    for (;;)
    {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
  }

  // ── State machine loop ──

  for (;;)
  {
    if (check_stop_signal())
    {
      continue;
    }

    switch (get_state())
    {

    // ────────────────────────────────────────────
    // IDLE — wait for wifi_manager_start()
    // ────────────────────────────────────────────
    case WIFI_ST_IDLE:
    {
      s_scan_attempt = 0;
      s_connected_ssid[0] = '\0';
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      set_state(WIFI_ST_SCANNING);
      break;
    }

    // ────────────────────────────────────────────
    // SCANNING — aggregated scan, match credentials
    // ────────────────────────────────────────────
    case WIFI_ST_SCANNING:
    {
      fire_event(WIFI_MGR_SCANNING);

      if (s_cred_count == 0)
      {
        ESP_LOGW(TAG, "No credentials stored");
        fire_event(WIFI_MGR_NO_MATCH);
        uint32_t delay = calc_backoff_ms(s_scan_attempt, SCAN_RETRY_BASE_MS, SCAN_RETRY_MAX_MS);
        ESP_LOGW(TAG, "Retry in %" PRIu32 "ms (attempt %d)...", delay, s_scan_attempt + 1);
        s_scan_attempt++;
        vTaskDelay(pdMS_TO_TICKS(delay));
        break;
      }

      // Multi-round aggregated scan
      memset(s_ap_list, 0, MAX_UNIQUE_APS * sizeof(wifi_ap_record_t));
      int ap_count = do_aggregated_scan(s_ap_list, MAX_UNIQUE_APS);
      ESP_LOGI(TAG, "Aggregated scan: %d unique APs after %d rounds", ap_count, SCAN_ROUNDS);
      fire_event(WIFI_MGR_SCAN_DONE);

      if (ap_count == 0)
      {
        ESP_LOGW(TAG, "No APs found in scan");
        uint32_t delay = calc_backoff_ms(s_scan_attempt, SCAN_RETRY_BASE_MS, SCAN_RETRY_MAX_MS);
        ESP_LOGW(TAG, "Retry in %" PRIu32 "ms (attempt %d)...", delay, s_scan_attempt + 1);
        s_scan_attempt++;
        vTaskDelay(pdMS_TO_TICKS(delay));
        break;
      }

      // Log merged results
      ESP_LOGI(TAG, "=== Merged WiFi networks (%d) ===", ap_count);
      for (int i = 0; i < ap_count; i++)
      {
        ESP_LOGI(TAG, "  [%2d] %-32s  RSSI:%d  CH:%d", i + 1, (const char *)s_ap_list[i].ssid, s_ap_list[i].rssi,
                 s_ap_list[i].primary);
      }

      // Match against credential hash map — build candidate list
      s_candidate_count = 0;
      for (int i = 0; i < ap_count && s_candidate_count < WIFI_MAX_CREDENTIALS; i++)
      {
        const char *ssid = (const char *)s_ap_list[i].ssid;
        if (ssid_map_contains(&s_cred_map, ssid))
        {
          bool dup = false;
          for (int k = 0; k < s_candidate_count; k++)
          {
            if (strcmp((const char *)s_ap_list[s_candidates[k]].ssid, ssid) == 0)
            {
              dup = true;
              break;
            }
          }
          if (!dup)
          {
            ESP_LOGI(TAG, "  Match: \"%s\" (RSSI: %d)", ssid, s_ap_list[i].rssi);
            s_candidates[s_candidate_count++] = i;
          }
        }
      }

      if (s_candidate_count == 0)
      {
        ESP_LOGW(TAG, "No scan results matched stored credentials");
        fire_event(WIFI_MGR_NO_MATCH);
        uint32_t delay = calc_backoff_ms(s_scan_attempt, SCAN_RETRY_BASE_MS, SCAN_RETRY_MAX_MS);
        ESP_LOGW(TAG, "Retry in %" PRIu32 "ms (attempt %d)...", delay, s_scan_attempt + 1);
        s_scan_attempt++;
        vTaskDelay(pdMS_TO_TICKS(delay));
        break;
      }

      ESP_LOGI(TAG, "Found %d matching networks", s_candidate_count);
      s_scan_attempt = 0; // Reset backoff on match found
      set_state(WIFI_ST_CONNECTING);
      break;
    }

    // ────────────────────────────────────────────
    // CONNECTING — try each candidate sequentially
    // ────────────────────────────────────────────
    case WIFI_ST_CONNECTING:
    {
      bool connected = false;

      for (int c = 0; c < s_candidate_count; c++)
      {
        if (check_stop_signal())
        {
          goto loop_continue;
        }

        int idx = s_candidates[c];
        const char *ssid = (const char *)s_ap_list[idx].ssid;
        const char *pass = ssid_map_get(&s_cred_map, ssid);

        ESP_LOGI(TAG, "Trying [%d/%d]: \"%s\"...", c + 1, s_candidate_count, ssid);

        if (try_connect_candidate(&s_ap_list[idx], pass))
        {
          ESP_LOGI(TAG, "Connected to \"%s\"!", ssid);
          connected = true;
          break;
        }
        else
        {
          ESP_LOGW(TAG, "Failed to connect to \"%s\"", ssid);
        }
      }

      if (connected)
      {
        set_state(WIFI_ST_VERIFYING);
      }
      else
      {
        // All candidates failed — backoff and re-scan (infinite)
        fire_event(WIFI_MGR_ALL_FAILED);
        uint32_t delay = calc_backoff_ms(s_scan_attempt, SCAN_RETRY_BASE_MS, SCAN_RETRY_MAX_MS);
        ESP_LOGW(TAG, "All candidates failed — retry in %" PRIu32 "ms (attempt %d)...", delay, s_scan_attempt + 1);
        s_scan_attempt++;
        vTaskDelay(pdMS_TO_TICKS(delay));
        set_state(WIFI_ST_SCANNING);
      }
      break;
    }

    // ────────────────────────────────────────────
    // VERIFYING — DNS probe to confirm internet
    // ────────────────────────────────────────────
    case WIFI_ST_VERIFYING:
    {
      do_dns_probe(); // Result logged inside; fallback always declares connected

      // Check if disconnected during probe
      EventBits_t bits = xEventGroupGetBits(s_event_group);
      if (bits & BIT_DISCONNECTED)
      {
        xEventGroupClearBits(s_event_group, BIT_DISCONNECTED);
        ESP_LOGW(TAG, "Disconnected during verification");
        fire_event(WIFI_MGR_DISCONNECTED);
        set_state(WIFI_ST_RECONNECTING);
        break;
      }
      if (bits & BIT_STOP)
      {
        break; // Will be caught by check_stop_signal at top of loop
      }

      // DNS probe done (success or fallback) — declare connected
      set_state(WIFI_ST_CONNECTED);
      fire_event(WIFI_MGR_CONNECTED);
      s_scan_attempt = 0;
      break;
    }

    // ────────────────────────────────────────────
    // CONNECTED — wait for disconnect or stop
    // ────────────────────────────────────────────
    case WIFI_ST_CONNECTED:
    {
      // Block until something happens
      EventBits_t bits =
          xEventGroupWaitBits(s_event_group, BIT_DISCONNECTED | BIT_STOP, pdTRUE, pdFALSE, portMAX_DELAY);

      if (bits & BIT_STOP)
      {
        xEventGroupClearBits(s_event_group, BIT_STOP | BIT_DISCONNECTED);
        set_state(WIFI_ST_IDLE);
      }
      else if (bits & BIT_DISCONNECTED)
      {
        fire_event(WIFI_MGR_DISCONNECTED);
        set_state(WIFI_ST_RECONNECTING);
      }
      break;
    }

    // ────────────────────────────────────────────
    // RECONNECTING — retry same SSID 3×, then re-scan
    // ────────────────────────────────────────────
    case WIFI_ST_RECONNECTING:
    {
      fire_event(WIFI_MGR_RECONNECTING);
      bool reconnected = false;

      for (int r = 0; r < RECONNECT_SAME_SSID_MAX; r++)
      {
        if (check_stop_signal())
        {
          goto loop_continue;
        }

        uint32_t delay =
            calc_backoff_ms(r, RECONNECT_BASE_DELAY_MS, RECONNECT_BASE_DELAY_MS << RECONNECT_SAME_SSID_MAX);
        ESP_LOGI(TAG, "Reconnect retry %d/%d \"%s\" (backoff: %" PRIu32 "ms)...", r + 1, RECONNECT_SAME_SSID_MAX,
                 s_connected_ssid, delay);

        xEventGroupClearBits(s_event_group, BIT_GOT_IP | BIT_DISCONNECTED);
        esp_wifi_connect();

        EventBits_t bits = xEventGroupWaitBits(s_event_group, BIT_GOT_IP | BIT_DISCONNECTED, pdTRUE, pdFALSE,
                                               pdMS_TO_TICKS(CONNECT_TIMEOUT_MS));

        if (bits & BIT_GOT_IP)
        {
          ESP_LOGI(TAG, "Reconnected to \"%s\"!", s_connected_ssid);
          reconnected = true;
          break;
        }

        // Wait before next retry
        vTaskDelay(pdMS_TO_TICKS(delay));
      }

      if (reconnected)
      {
        set_state(WIFI_ST_VERIFYING);
      }
      else
      {
        ESP_LOGW(TAG, "Reconnect exhausted — falling back to full scan");
        s_scan_attempt = 0;
        set_state(WIFI_ST_SCANNING);
      }
      break;
    }

    } // switch

  loop_continue:;
  } // for(;;)
}

// ============================================================
// Public API
// ============================================================

esp_err_t wifi_manager_init(void)
{
  ESP_LOGI(TAG, "Initializing WiFi manager...");

  // NVS (must be first)
  ESP_ERROR_CHECK(wifi_cred_store_init());

  // Provision compiled credentials
  wifi_cred_store_provision();

  // Network interface
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  s_sta_netif = esp_netif_create_default_wifi_sta();

  // WiFi driver
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Synchronization primitives
  s_event_group = xEventGroupCreate();
  s_mutex = xSemaphoreCreateMutex();

  // Event handlers — bits only
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

  // Station mode
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Create persistent task (starts in IDLE, waits for notification)
  BaseType_t xret = xTaskCreatePinnedToCore(wifi_task, "wifi_mgr",
                                            4096 * 2, // 8KB stack
                                            NULL, 3,  // Priority 3
                                            &s_task_handle,
                                            0); // Pin to core 0 (WiFi core)

  if (xret != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create WiFi task");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "WiFi manager initialized");
  return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
  if (get_state() != WIFI_ST_IDLE)
  {
    ESP_LOGW(TAG, "Cannot start — state is %s", state_name(get_state()));
    return ESP_ERR_INVALID_STATE;
  }

  xTaskNotifyGive(s_task_handle);
  return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
  return get_state() == WIFI_ST_CONNECTED;
}

esp_err_t wifi_manager_stop(void)
{
  ESP_LOGI(TAG, "Stopping WiFi...");
  xEventGroupSetBits(s_event_group, BIT_STOP);
  esp_wifi_disconnect();
  return ESP_OK;
}

void wifi_manager_set_callback(wifi_event_cb_t cb)
{
  s_callback = cb;
}

wifi_state_t wifi_manager_get_state(void)
{
  return get_state();
}

const char *wifi_manager_get_ssid(void)
{
  return s_connected_ssid[0] ? s_connected_ssid : NULL;
}

esp_err_t wifi_cred_add(const char *ssid, const char *password)
{
  return wifi_cred_store_add(ssid, password);
}

esp_err_t wifi_cred_remove(const char *ssid)
{
  return wifi_cred_store_remove(ssid);
}
