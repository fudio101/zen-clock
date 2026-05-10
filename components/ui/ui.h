// SPDX-License-Identifier: MIT
// ZenClock UI — public API

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

  /**
   * @brief Initialize the UI — creates screen, theme, and all widget modules.
   *
   * Must be called inside lvgl_port_lock()/unlock().
   */
  void ui_init(void);

#ifdef __cplusplus
}
#endif
