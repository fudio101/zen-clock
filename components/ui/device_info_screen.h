// SPDX-License-Identifier: MIT
// ZenClock — System Info screen (read-only, scrollable)

#pragma once

#include "lvgl.h"
#include "microlink.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void device_info_screen_create(lv_obj_t *parent);
  void device_info_screen_scroll_up(void);
  void device_info_screen_scroll_down(void);
  void device_info_screen_destroy(void);

  /** Set the MicroLink handle for Tailscale status rows. Call after microlink_init(). NULL = disabled. */
  void device_info_screen_set_ml(microlink_t *ml);

#ifdef __cplusplus
}
#endif
