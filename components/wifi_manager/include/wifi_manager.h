// SPDX-License-Identifier: MIT
// ZenClock WiFi Manager — Public API

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "esp_err.h"
#include <stdbool.h>

// Maximum stored WiFi credentials
#define WIFI_MAX_CREDENTIALS 16

  // ============================================================
  // State Machine
  // ============================================================

  typedef enum
  {
    WIFI_ST_IDLE,         // Initialized, not running. Only via stop()
    WIFI_ST_SCANNING,     // Aggregated scan in progress
    WIFI_ST_CONNECTING,   // Trying candidate APs
    WIFI_ST_VERIFYING,    // Got IP, checking internet connectivity
    WIFI_ST_CONNECTED,    // Verified online, operational
    WIFI_ST_RECONNECTING, // Lost connection, retrying same SSID
  } wifi_state_t;

  // ============================================================
  // Events
  // ============================================================

  typedef enum
  {
    WIFI_MGR_SCANNING,     // Started scanning for APs
    WIFI_MGR_CONNECTING,   // Trying a candidate
    WIFI_MGR_GOT_IP,       // Got IP but not yet verified online
    WIFI_MGR_CONNECTED,    // Verified online (DNS probe OK)
    WIFI_MGR_DISCONNECTED, // Lost connection
    WIFI_MGR_RECONNECTING, // Retrying same SSID before re-scan
    WIFI_MGR_SCAN_DONE,    // WiFi scan complete
    WIFI_MGR_NO_MATCH,     // No scan result matched stored creds
    WIFI_MGR_ALL_FAILED,   // One scan cycle failed — retrying (informational)
  } wifi_manager_event_t;

  typedef void (*wifi_event_cb_t)(wifi_manager_event_t event);

  // ============================================================
  // Lifecycle
  // ============================================================

  /**
   * @brief Initialize WiFi subsystem.
   *
   * Inits NVS, netif, event loop, WiFi driver (STA mode),
   * provisions compiled credentials into NVS if changed,
   * and creates the persistent WiFi task (in IDLE state).
   */
  esp_err_t wifi_manager_init(void);

  /**
   * @brief Start async scan → match → connect sequence.
   *
   * Wakes the persistent WiFi task to begin scanning.
   * The task retries indefinitely with exponential backoff
   * until connected or wifi_manager_stop() is called.
   */
  esp_err_t wifi_manager_start(void);

  /**
   * @brief Check if currently connected and verified online.
   */
  bool wifi_manager_is_connected(void);

  /**
   * @brief Stop WiFi and return to IDLE state.
   *
   * Signals the task to stop, disconnects, and stops WiFi driver.
   * Call wifi_manager_start() to restart.
   */
  esp_err_t wifi_manager_stop(void);

  /**
   * @brief Set callback for WiFi events.
   */
  void wifi_manager_set_callback(wifi_event_cb_t cb);

  /**
   * @brief Get current state machine state.
   */
  wifi_state_t wifi_manager_get_state(void);

  /**
   * @brief Get SSID of current/last connection.
   * @return SSID string, or NULL if never connected.
   */
  const char *wifi_manager_get_ssid(void);

  // ============================================================
  // Credential Management (runtime)
  // ============================================================

  /**
   * @brief Add a credential to NVS.
   */
  esp_err_t wifi_cred_add(const char *ssid, const char *password);

  /**
   * @brief Remove a credential from NVS.
   */
  esp_err_t wifi_cred_remove(const char *ssid);

#ifdef __cplusplus
}
#endif
