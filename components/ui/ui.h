// SPDX-License-Identifier: MIT
// ZenClock UI — public API

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Initialize the LVGL UI.
   * Creates the main screen, clock face, and status bar.
   * MUST be called while holding the LVGL port lock.
   * @param is_light true to start in light theme, false for dark theme.
   */
  void ui_init(bool is_light);

  /**
   * @brief Set the UI theme dynamically.
   * @param is_light true to switch to light theme, false for dark theme.
   */
  void ui_set_theme(bool is_light);

  /**
   * @brief Query the current theme.
   * @return true if light theme is active, false if dark.
   */
  bool ui_is_light_theme(void);

  /**
   * @brief Apply the theme background color to a screen object.
   * Call after lv_obj_create(NULL) for every new screen.
   */
  void ui_apply_screen_bg(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif
