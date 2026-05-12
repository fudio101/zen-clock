// SPDX-License-Identifier: MIT
// ZenClock — Clock face widget interface

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Create the clock face widget on the given parent.
   *
   * Creates time (HH:MM:SS) and date (DD/MM/YYYY) displays with a 1-second
   * LVGL timer for automatic updates. Timer fires immediately on first tick.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   * Timezone must be configured (setenv TZ) before calling.
   */
  void clock_face_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
