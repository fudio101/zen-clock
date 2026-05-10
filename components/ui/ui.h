// SPDX-License-Identifier: MIT
// ZenClock UI — Hand-written LVGL interface (no SquareLine Studio)

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

  // ============================================================
  // Screen
  // ============================================================
  extern lv_obj_t *ui_screen_main;

  // ============================================================
  // Battery widgets (updated by battery task in main.c)
  // ============================================================
  extern lv_obj_t *ui_bat_icon_label;
  extern lv_obj_t *ui_bat_pct_label;

  // ============================================================
  // Content widgets
  // ============================================================
  extern lv_obj_t *ui_clock_label;

  // ============================================================
  // Lifecycle
  // ============================================================

  /**
   * @brief Initialize the UI — creates all screens and widgets.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void ui_init(void);

#ifdef __cplusplus
}
#endif
