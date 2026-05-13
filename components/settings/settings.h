// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Initialize the NVS flash.
   * Must be called early in app_main before reading/writing settings.
   */
  void settings_init(void);

  /**
   * @brief Get the stored theme configuration.
   * @return true if light theme, false if dark theme (default).
   */
  bool settings_get_theme_light(void);

  /**
   * @brief Store the theme configuration to NVS.
   * @param is_light true for light theme, false for dark theme.
   */
  void settings_set_theme_light(bool is_light);

  /**
   * @brief Get the stored brightness percentage.
   * @return Brightness 0–100 (default: 100 if not set).
   */
  uint8_t settings_get_brightness(void);

  /**
   * @brief Store brightness percentage to NVS.
   * @param percent Brightness 0–100 (clamped if exceeds 100).
   */
  void settings_set_brightness(uint8_t percent);

#ifdef __cplusplus
}
#endif
