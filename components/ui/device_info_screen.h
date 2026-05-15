// SPDX-License-Identifier: MIT
// ZenClock — System Info screen (read-only, scrollable)

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void device_info_screen_create(lv_obj_t *parent);
  void device_info_screen_scroll_up(void);
  void device_info_screen_scroll_down(void);
  void device_info_screen_destroy(void);

#ifdef __cplusplus
}
#endif
