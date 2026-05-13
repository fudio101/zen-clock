// SPDX-License-Identifier: MIT
// ZenClock — Status bar widget interface

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

  // ============================================================
  // WiFi status for UI display (WiFi layer only)
  // ============================================================

  typedef enum
  {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_SCANNING, // Scan in progress (between retries)
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_VERIFYING,    // Got IP, checking internet
    WIFI_STATUS_CONNECTED,    // Verified online
    WIFI_STATUS_PROVISIONING, // BLE provisioning active — waiting for credentials
  } wifi_status_t;

  // ============================================================
  // SNTP status for UI display (NTP sync layer)
  // ============================================================

  typedef enum
  {
    SNTP_STATUS_IDLE,    // Not started yet
    SNTP_STATUS_SYNCING, // NTP sync in progress
    SNTP_STATUS_SYNCED,  // Time synchronized
    SNTP_STATUS_FAILED,  // Sync failed / timed out
  } sntp_status_t;

  /**
   * @brief Create the status bar on the given parent.
   *
   * Creates SNTP indicator + WiFi indicator + battery icon + percentage label
   * in the top-right corner with a 30-second LVGL timer for
   * automatic battery updates. Timer fires immediately on first tick.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void status_bar_create(lv_obj_t *parent);

  /**
   * @brief Update WiFi status indicator.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void status_bar_set_wifi_status(wifi_status_t status);

  /**
   * @brief Update SNTP status indicator.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void status_bar_set_sntp_status(sntp_status_t status);

  /**
   * @brief Destroy the status bar and its battery timer.
   *
   * Must be called before deleting the parent screen to avoid
   * stale timer references. Safe to call if not created.
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void status_bar_destroy(void);

#ifdef __cplusplus
}
#endif
