// SPDX-License-Identifier: MIT
// ZenClock — BLE Provisioning overlay screen

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Show full-screen BLE provisioning overlay with QR code.
   * Must be called while holding the LVGL port lock.
   * @param device_name BLE advertisement name (e.g. "PROV_ZenClock_A1B2")
   */
  void prov_screen_show(const char *device_name);

  /**
   * @brief Remove the provisioning overlay, revealing the clock screen.
   * Must be called while holding the LVGL port lock.
   */
  void prov_screen_hide(void);

#ifdef __cplusplus
}
#endif
