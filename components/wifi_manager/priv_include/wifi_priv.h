// SPDX-License-Identifier: MIT
// ZenClock WiFi Manager — Internal declarations
// DO NOT include from outside this component.

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // ============================================================
  // Buffer sizes (keep in sync with WIFI_SSID_MAX_LEN / WIFI_PASS_MAX_LEN)
  // ============================================================

#define SSID_MAX_LEN 33 // IEEE 802.11 max SSID + null
#define PASS_MAX_LEN 65 // WPA max 63 chars + null

  // ============================================================
  // Scan — multiple rounds merged by BSSID
  // ============================================================

#define SCAN_ROUNDS 3           // Number of scan rounds to aggregate
#define SCAN_MIN_TIME_MS 100    // Min active dwell time per channel (ms)
#define SCAN_MAX_TIME_MS 300    // Max active dwell time per channel (ms)
#define SCAN_INTER_DELAY_MS 200 // Pause between scan rounds (ms)
#define MAX_UNIQUE_APS 64       // Max merged AP entries

  // ============================================================
  // Connection — per-attempt timeout
  // ============================================================

#define CONNECT_TIMEOUT_MS 15000 // Connection attempt timeout (ms)

  // ============================================================
  // DNS connectivity probe — verify internet after Got IP
  // ============================================================

#define DNS_PROBE_HOST "pool.ntp.org"
#define DNS_PROBE_MAX 5            // Max probe attempts
#define DNS_PROBE_INTERVAL_MS 2000 // Between probes (ms)

  // ============================================================
  // Single-credential loader (defined in wifi_credentials.c)
  // ============================================================

  bool wifi_cred_load(char *out_ssid, size_t ssid_len, char *out_pass, size_t pass_len);

#ifdef __cplusplus
}
#endif
