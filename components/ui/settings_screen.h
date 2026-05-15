// SPDX-License-Identifier: MIT
// ZenClock — Settings screen widget interface

#pragma once

#include "lvgl.h"
#include "nav.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /** Create settings screen content on the given parent. */
  void settings_screen_create(lv_obj_t *parent);

  void settings_screen_focus_prev(void);
  void settings_screen_focus_next(void);
  int settings_screen_get_focus(void);
  void settings_screen_set_focus(int index);

  /** Check if item at index is an action (not editable). */
  bool settings_screen_is_action_item(int index);

  /**
   * @brief Execute an action item (e.g., Reset WiFi).
   * @param index   Item index
   * @param cb      External callback (from app_handlers via nav)
   */
  void settings_screen_execute_action(int index, nav_action_cb_t cb);

  /** Enter edit mode for the focused item. */
  void settings_screen_enter_edit(int index);

  /** Exit edit mode. */
  void settings_screen_exit_edit(void);

  /** Increase/next value in edit mode. */
  void settings_screen_edit_increase(void);

  /** Decrease/prev value in edit mode. */
  void settings_screen_edit_decrease(void);

#ifdef __cplusplus
}
#endif
