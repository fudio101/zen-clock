// SPDX-License-Identifier: MIT
// ZenClock — Navigation state machine

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    NAV_ACTION_UP,     // BOOT short — move up / increase
    NAV_ACTION_DOWN,   // IO14 short — move down / decrease
    NAV_ACTION_SELECT, // BOOT long  — select / enter
    NAV_ACTION_BACK,   // IO14 long  — back
  } nav_action_t;

  typedef void (*nav_action_cb_t)(void);

  /**
   * @brief Initialize navigation. Sets up clock screen as default.
   * Must be called after ui_init() while holding LVGL lock.
   */
  void nav_init(void);

  /**
   * @brief Process a navigation action from button input.
   * Must be called while holding LVGL lock.
   */
  void nav_handle_action(nav_action_t action);

  /**
   * @brief Register callback for "Reset Wi-Fi" action.
   * Called by app_handlers to keep ui component decoupled from wi-fi/ble.
   */
  void nav_register_reset_wifi_cb(nav_action_cb_t cb);

  /**
   * @brief Register callback for "Sleep Now" action.
   * Called by app_handlers to keep ui component decoupled from deep_sleep.
   */
  void nav_register_sleep_cb(nav_action_cb_t cb);

  /**
   * @brief Register callback for "NTP Resync" action.
   * Called by app_handlers to keep ui component decoupled from sntp_sync.
   */
  void nav_register_ntp_resync_cb(nav_action_cb_t cb);

#ifdef __cplusplus
}
#endif
