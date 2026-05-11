// SPDX-License-Identifier: MIT
// ZenClock WiFi Manager — Internal declarations
// DO NOT include from outside this component.

#pragma once

#include <stdbool.h>
#include <stdint.h>

// ============================================================
// SSID Hash Map — O(1) lookup for scan matching
// ============================================================

#define SSID_MAP_CAPACITY 32 // Buckets (> 2× WIFI_MAX_CREDENTIALS)
#define SSID_MAX_LEN 33      // IEEE 802.11 max SSID + null
#define PASS_MAX_LEN 65      // WPA max 63 chars + null

// ============================================================
// Scan stability — multiple rounds merged by BSSID
// ============================================================

#define SCAN_ROUNDS 3           // Number of scan rounds to aggregate
#define SCAN_MIN_TIME_MS 100    // Min active dwell time per channel (ms)
#define SCAN_MAX_TIME_MS 300    // Max active dwell time per channel (ms)
#define SCAN_INTER_DELAY_MS 200 // Pause between scan rounds (ms)
#define MAX_UNIQUE_APS 64       // Max merged AP entries

// ============================================================
// Connection — per-candidate timeout
// ============================================================

#define CONNECT_TIMEOUT_MS 15000 // Per-candidate connection timeout (ms)

// ============================================================
// Scan→connect infinite retry with exponential backoff
// ============================================================

#define SCAN_RETRY_BASE_MS 2000 // First backoff: 2s
#define SCAN_RETRY_MAX_MS 30000 // Backoff cap: 30s
// Delay = min(SCAN_RETRY_BASE_MS << attempt, SCAN_RETRY_MAX_MS)

// ============================================================
// Reconnect — retry same SSID, then re-scan
// ============================================================

#define RECONNECT_SAME_SSID_MAX 3    // Retry same SSID before re-scan
#define RECONNECT_BASE_DELAY_MS 2000 // Exponential: 2s, 4s, 8s

// ============================================================
// DNS connectivity probe — verify internet after Got IP
// ============================================================

#define DNS_PROBE_HOST "pool.ntp.org"
#define DNS_PROBE_MAX 5            // Max probe attempts
#define DNS_PROBE_INTERVAL_MS 2000 // Between probes

// ============================================================
// SSID Hash Map types
// ============================================================

typedef struct
{
  char ssid[SSID_MAX_LEN];
  char password[PASS_MAX_LEN];
  bool occupied;
} ssid_map_entry_t;

typedef struct
{
  ssid_map_entry_t buckets[SSID_MAP_CAPACITY];
  int count;
} ssid_map_t;

void ssid_map_init(ssid_map_t *map);
bool ssid_map_put(ssid_map_t *map, const char *ssid, const char *password);
const char *ssid_map_get(const ssid_map_t *map, const char *ssid);
bool ssid_map_contains(const ssid_map_t *map, const char *ssid);
void ssid_map_clear(ssid_map_t *map);

// ============================================================
// NVS Credential Store
// ============================================================

#include "esp_err.h"

esp_err_t wifi_cred_store_init(void);
esp_err_t wifi_cred_store_provision(void); // From compiled array
int wifi_cred_store_load_all(ssid_map_t *map);
esp_err_t wifi_cred_store_add(const char *ssid, const char *password);
esp_err_t wifi_cred_store_remove(const char *ssid);
void wifi_cred_store_deinit(void);
