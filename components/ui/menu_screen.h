// SPDX-License-Identifier: MIT
// ZenClock — Menu screen widget interface

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /** Create menu screen content on the given parent (below status bar). */
  void menu_screen_create(lv_obj_t *parent);

  /** Move focus to previous item. Stops at first. */
  void menu_screen_focus_prev(void);

  /** Move focus to next item. Stops at last. */
  void menu_screen_focus_next(void);

  /** Get current focus index. */
  int menu_screen_get_focus(void);

  /** Set focus to a specific index (for restoring position). */
  void menu_screen_set_focus(int index);

#ifdef __cplusplus
}
#endif
